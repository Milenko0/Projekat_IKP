#define _CRT_NONSTDC_NO_DEPRECATE
#define _CRT_SECURE_NO_WARNINGS
#define WIN32_LEAN_AND_MEAN
#define _WINSOCK_DEPRECATED_NO_WARNINGS
#define WORKER_TERMINATE -1

#include <windows.h>
#include <winsock2.h>
#include <ws2tcpip.h>
#include <stdlib.h>
#include <stdio.h>
#include <conio.h>

#include <ws2tcpip.h>
#include <stdlib.h>
#include <stdio.h>
#include <time.h>


#include "../Common/Lista.cpp";
#include "../Common/Measurement.cpp";

#pragma comment (lib, "Ws2_32.lib")
#pragma comment (lib, "Mswsock.lib")
#pragma comment (lib, "AdvApi32.lib")
#pragma comment(lib,"WS2_32")


#define DEFAULT_PORT "5555"
#define MAX_CLIENTS 10
#define MAX_THREADS 16
#define TIMEVAL_SEC 0
#define TIMEVAL_USEC 100

#define SAFE_DELETE_HANDLE(a)  if(a){CloseHandle(a);}

bool InitializeWindowsSockets();
SOCKET InitializeListenSocket();
DWORD WINAPI Listen(LPVOID);
void SetAcceptedSocketsInvalid();
void ProcessMessages();
void ProcessMeasurement(Measurement*);
void Cleanup();
void UpdateSubscribers(Measurement*, NODE*);
void SendToNewSubscriber(SOCKET, NODE*);
bool InitCriticalSections();
bool TCPSend(SOCKET connectSocket, Measurement measurement);
int TCPReceive(SOCKET connectSocket, char* recvbuf, size_t len);
DWORD WINAPI DoWork(LPVOID);
DWORD WINAPI InitWorkerThreads(LPVOID);
bool Work(int);

fd_set readfds;
SOCKET listenSocket = INVALID_SOCKET;
SOCKET acceptedSockets[MAX_CLIENTS];
timeval timeVal;

CRITICAL_SECTION CSAnalogData;
CRITICAL_SECTION CSStatusData;
CRITICAL_SECTION CSAnalogSubs;
CRITICAL_SECTION CSStatusSubs;
CRITICAL_SECTION CSWorkerTasks;


NODE* statusData = NULL;
NODE* analogData = NULL;
NODE* statusSubscribers = NULL;
NODE* analogSubscribers = NULL;
NODE* workerTasks = NULL;


HANDLE listenHandle;
bool terminateListen = false;
HANDLE workerManagerHandle;
HANDLE workerHandles[MAX_THREADS];

typedef struct worker_data {
    int i;
}WorkerData;


DWORD WINAPI InitWorkerThreads(LPVOID params) {
    for (int i = 0; i < MAX_THREADS; ++i) {
        workerHandles[i] = CreateThread(NULL, 0, &DoWork, (LPVOID)0, 0, NULL);
        if (workerHandles[i] == 0) {
            printf("WorkerManager nije uspeo napraviti worker thread.");
            return false;
        }
    }

    if (WaitForMultipleObjects(MAX_THREADS, workerHandles, TRUE, INFINITE) != WAIT_OBJECT_0 + MAX_THREADS - 1) {
        printf("Workers finished\n");
    }

    return true;
}


bool InitializeWindowsSockets()
{
    WSADATA wsaData;
    // Initialize windows sockets library for this process
    if (WSAStartup(MAKEWORD(2, 2), &wsaData) != 0)
    {
        printf("WSAStartup failed with error: %d\n", WSAGetLastError());
        return false;
    }
    return true;
}

SOCKET InitializeListenSocket() {

    addrinfo* resultingAddress = NULL;
    addrinfo hints;

    memset(&hints, 0, sizeof(hints));
    hints.ai_family = AF_INET;       // IPv4 address
    hints.ai_socktype = SOCK_STREAM; // Provide reliable data streaming
    hints.ai_protocol = IPPROTO_TCP; // Use TCP protocol
    hints.ai_flags = AI_PASSIVE;

    // Resolve the server address and port
    int iResult = getaddrinfo(NULL, DEFAULT_PORT, &hints, &resultingAddress);
    if (iResult != 0)
    {
        printf("getaddrinfo failed with error: %d\n", iResult);
        WSACleanup();
        return 1;
    }

    // Create a SOCKET for connecting to server
    // IPv4 address famly|stream socket | TCP protocol
    listenSocket = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);

    if (listenSocket == INVALID_SOCKET)
    {
        printf("socket failed with error: %ld\n", WSAGetLastError());
        freeaddrinfo(resultingAddress);

        WSACleanup();
        return INVALID_SOCKET;
    }

    // Setup the TCP listening socket - bind port number and local address 
    // to socket
    iResult = bind(listenSocket, resultingAddress->ai_addr, (int)resultingAddress->ai_addrlen);
    if (iResult == SOCKET_ERROR)
    {
        printf("bind failed with error: %d\n", WSAGetLastError());
        freeaddrinfo(resultingAddress);
        WSACleanup();
        return INVALID_SOCKET;
    }
    freeaddrinfo(resultingAddress);
    unsigned long mode = 1;
    iResult = ioctlsocket(listenSocket, FIONBIO, &mode);
    if (iResult != NO_ERROR) {
        printf("ioctlsocket failed with error: %ld\n", iResult);
        return INVALID_SOCKET;
    }
    FD_ZERO(&readfds);
    FD_SET(listenSocket, &readfds);
    timeVal.tv_sec = TIMEVAL_SEC;
    timeVal.tv_usec = TIMEVAL_USEC;
    return listenSocket;
}

bool InitCriticalSections() {
    InitializeCriticalSection(&CSAnalogData);
    InitializeCriticalSection(&CSStatusData);
    InitializeCriticalSection(&CSAnalogSubs);
    InitializeCriticalSection(&CSStatusSubs);
    InitializeCriticalSection(&CSWorkerTasks);

    return true;
}

DWORD WINAPI Listen(LPVOID param) {
    int iResult = listen(listenSocket, SOMAXCONN);
    if (iResult == SOCKET_ERROR)
    {
        printf("listen failed with error: %d\n", WSAGetLastError());
        closesocket(listenSocket);
        Cleanup();
        return 0;
    }
    printf("Listening...\n");

    while (1)
    {
        FD_ZERO(&readfds);
        FD_SET(listenSocket, &readfds);

        for (int i = 0; i < MAX_CLIENTS; i++) {
            if (acceptedSockets[i] != INVALID_SOCKET)
                FD_SET(acceptedSockets[i], &readfds);
        }

        int value = select(0, &readfds, NULL, NULL, &timeVal);

        if (value == 0) {
            if (terminateListen) {
                printf("Stopped listening.\n");
                return true;
            }
        }
        else if (value == SOCKET_ERROR) {
            continue;
        }
        else {
            if (FD_ISSET(listenSocket, &readfds)) {
                int  i;
                for (i = 0; i < MAX_CLIENTS; i++) {
                    if (acceptedSockets[i] == INVALID_SOCKET) {
                        acceptedSockets[i] = accept(listenSocket, NULL, NULL);
                        if (acceptedSockets[i] == INVALID_SOCKET)
                        {
                            printf("accept failed with error: %d\n", WSAGetLastError());
                            closesocket(acceptedSockets[i]);
                            acceptedSockets[i] = INVALID_SOCKET;
                            return 0;
                        }
                        break;
                    }
                }
                if (i == MAX_CLIENTS) {
                    printf("Nema mesta za jos jednu konekciju.\n");
                }
            }
            ProcessMessages();
        }
    }

}

bool Work(int i) {
    char* data = (char*)malloc(sizeof(Measurement));
    int succes = TCPReceive(acceptedSockets[i], data, sizeof(Measurement));

    if (succes == 0) {
        EnterCriticalSection(&CSAnalogSubs);
        DeleteNode(&analogSubscribers, &acceptedSockets[i], sizeof(SOCKET));
        LeaveCriticalSection(&CSAnalogSubs);

        EnterCriticalSection(&CSStatusSubs);
        DeleteNode(&statusSubscribers, &acceptedSockets[i], sizeof(SOCKET));
        LeaveCriticalSection(&CSStatusSubs);

        closesocket(acceptedSockets[i]);
        acceptedSockets[i] = INVALID_SOCKET;
        free(data);
        return false;
    }
    else if (succes == 2) {
        free(data);
        return false;
    }

    SOCKET* ptr = &acceptedSockets[i];
    if (data[0] == 'a') {
        EnterCriticalSection(&CSAnalogSubs);
        ListPushAtStart(&analogSubscribers, ptr, sizeof(SOCKET));
        //SendToNewSubscriber(acceptedSockets[i], analogData);
        LeaveCriticalSection(&CSAnalogSubs);
        printf("Klijent %d se preplatio na analogni topic\n", i);
    }
    else if (data[0] == 's') {
        EnterCriticalSection(&CSStatusSubs);
        ListPushAtStart(&statusSubscribers, ptr, sizeof(SOCKET));
        //SendToNewSubscriber(acceptedSockets[i], statusData);
        LeaveCriticalSection(&CSStatusSubs);
        printf("Klijent %d se preplatio na statusni topic\n", i);
    }
    else if (data[0] == 'o') {
        EnterCriticalSection(&CSStatusSubs);
        ListPushAtStart(&statusSubscribers, ptr, sizeof(SOCKET));
        //SendToNewSubscriber(acceptedSockets[i], statusData);
        LeaveCriticalSection(&CSStatusSubs);
        EnterCriticalSection(&CSAnalogSubs);
        ListPushAtStart(&analogSubscribers, ptr, sizeof(SOCKET));
        //SendToNewSubscriber(acceptedSockets[i], analogData);
        LeaveCriticalSection(&CSAnalogSubs);
        printf("Klijent %d se preplatio na statusni i analogni topic\n", i);
    }
    else {
        Measurement* newMeasurement = (Measurement*)malloc(sizeof(Measurement));
        memcpy(newMeasurement, data, sizeof(Measurement));
        ProcessMeasurement(newMeasurement);
        free(newMeasurement);
    }
    free(data);
    return true;
}

void SetAcceptedSocketsInvalid() {
    for (int i = 0; i < MAX_CLIENTS; i++) {
        acceptedSockets[i] = INVALID_SOCKET;
    }
}

void ProcessMessages() {
    for (int i = 0; i < MAX_CLIENTS; i++) {
        if (FD_ISSET(acceptedSockets[i], &readfds)) {
            WorkerData* taskData = (WorkerData*)malloc(sizeof(WorkerData));
            taskData->i = i;
            EnterCriticalSection(&CSWorkerTasks);
            ListPushAtStart(&workerTasks, taskData, sizeof(WorkerData));
            LeaveCriticalSection(&CSWorkerTasks);
            free(taskData);
        }
    }
}

void ProcessMeasurement(Measurement* measure) {
    switch (measure->topic)
    {
    case Analog:
        EnterCriticalSection(&CSAnalogData);
        ListPushAtStart(&analogData, measure, sizeof(Measurement));
        LeaveCriticalSection(&CSAnalogData);
        UpdateSubscribers(measure, analogSubscribers);
        break;
    case Status:
        EnterCriticalSection(&CSStatusData);
        ListPushAtStart(&statusData, measure, sizeof(Measurement));
        LeaveCriticalSection(&CSStatusData);
        UpdateSubscribers(measure, statusSubscribers);
        break;
    default:
        printf("Topik %d nije podrzan.", measure->topic);
        break;
    }
}

void Cleanup() {
    printf("Gasenje...\n");
    terminateListen = true;
    if (listenHandle) {
        WaitForSingleObject(listenHandle, INFINITE);
    }
    WorkerData* terminate = (WorkerData*)malloc(sizeof(WorkerData));
    terminate->i = WORKER_TERMINATE;

    EnterCriticalSection(&CSWorkerTasks);
    for (int i = 0; i < MAX_THREADS; ++i) {
        ListPushAtStart(&workerTasks, terminate, sizeof(WorkerData));
    }
    LeaveCriticalSection(&CSWorkerTasks);

    free(terminate);

    if (workerManagerHandle) {
        WaitForSingleObject(workerManagerHandle, INFINITE);
    }

    SAFE_DELETE_HANDLE(listenHandle);
    SAFE_DELETE_HANDLE(workerManagerHandle);
    for (int i = 0; i < MAX_THREADS; ++i) {
        SAFE_DELETE_HANDLE(workerHandles[i]);
    }

    DeleteCriticalSection(&CSStatusData);
    DeleteCriticalSection(&CSAnalogData);
    DeleteCriticalSection(&CSStatusSubs);
    DeleteCriticalSection(&CSAnalogSubs);
    DeleteCriticalSection(&CSWorkerTasks);

    FreeList(&statusData);
    FreeList(&analogData);
    FreeList(&statusSubscribers);
    FreeList(&analogSubscribers);
    FreeList(&workerTasks);

    closesocket(listenSocket);
    for (int i = 0; i < MAX_CLIENTS; ++i) {
        if (acceptedSockets[i] != INVALID_SOCKET) {
            closesocket(acceptedSockets[i]);
        }
    }
    WSACleanup();
    printf("Uspesno ciscenje, pritisnite enter za zatvaranje.\n");
    getchar();
    return;
}

void UpdateSubscribers(Measurement* measure, Node* list) {
    Node* temp = list;
    if (temp == NULL) {
        return;
    }
    while (temp != NULL) {
        SOCKET s;
        memcpy(&s, temp->data, sizeof(SOCKET));
        TCPSend(s, *measure);
        temp = temp->next;
    }
}

void SendToNewSubscriber(SOCKET sub, NODE* dataHead) {
    Node* temp = dataHead;
    if (temp == NULL) {
        return;
    }
    Measurement* data = (Measurement*)malloc(sizeof(Measurement));
    while (temp != NULL) {
        memcpy(data, temp->data, sizeof(Measurement));
        TCPSend(sub, *data);
        temp = temp->next;
    }
    free(data);
}

DWORD WINAPI DoWork(LPVOID params) {
    bool execute = false;
    while (true) {
        if (workerTasks == NULL) {
            Sleep(10);
        }
        else {
            WorkerData* wData = (WorkerData*)malloc(sizeof(WorkerData));
            EnterCriticalSection(&CSWorkerTasks);
            if (workerTasks != NULL) {
                memcpy(wData, workerTasks->data, sizeof(WorkerData));
                DeleteNode(&workerTasks, workerTasks->data, sizeof(WorkerData));
                execute = true;
            }
            LeaveCriticalSection(&CSWorkerTasks);
            if (execute) {
                if (wData->i == WORKER_TERMINATE) {
                    free(wData);
                    return true;
                }
                Work(wData->i);
                execute = false;
            }
            free(wData);
        }

    }
    return false;
}

int TCPReceive(SOCKET connectSocket, char* recvbuf, size_t len) {
    int iResult = recv(connectSocket, (char*)recvbuf, len, 0);
    if (iResult > 0)
    {
        return 1;
    }
    else if (WSAGetLastError() == WSAEWOULDBLOCK) {
        return 2;
    }
    else if (iResult == SOCKET_ERROR)
    {
        return 0;
    }
    else {
        return 0;
    }
}

bool TCPSend(SOCKET connectSocket, Measurement measurement) {
    while (true) {
        char* data = (char*)malloc(sizeof(Measurement));
        memcpy(data, (const void*)&measurement, sizeof(Measurement));
        int iResult = send(connectSocket, data, sizeof(Measurement), 0);
        free(data);
        if (WSAGetLastError() == WSAEWOULDBLOCK) {
            continue;
        }
        else if (iResult == SOCKET_ERROR)
        {
            return false;
        }
        else {
            return true;
        }
    }
}