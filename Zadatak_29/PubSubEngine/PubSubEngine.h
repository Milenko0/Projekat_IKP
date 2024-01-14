#pragma once
#define _CRT_NONSTDC_NO_DEPRECATE
#define _CRT_SECURE_NO_WARNINGS
#include <WinSock2.h>
#include <WS2tcpip.h>
#include <stdio.h>
#include <stdlib.h>
#include <conio.h>
#include <time.h>
#include "../Common/Measurement.cpp";
#include "../Common/Lista.cpp";
#define _WINSOCK_DEPRECATED_NO_WARNINGS 

#define DEFAULT_BUFLEN 524 
#define DEFAULT_PORT "5555"
#define MAX_CLIENTS 10
#define TIMEVAL_SEC 0
#define TIMEVAL_USEC 0
#pragma comment(lib, "ws2_32.lib")
#define SAFE_DELETE_HANDLE(a)  if(a){CloseHandle(a);}

//int Init();
bool InitCriticalSections();
DWORD WINAPI Listen(LPVOID param);
bool InitializeWindowsSockets();
SOCKET InitializeListenSocket();
void SetAcceptedSocketsInvalid();
void ProcessMessages();
void ProcessMeasurement(Measurement * measure);
void Cleanup();
void UpdateSubscribers(Measurement * measure, Node * list);
void SendToNewSubscriber(SOCKET subscribe, NODE * dataHead);

fd_set readfds;
SOCKET listenSocket = INVALID_SOCKET;
SOCKET acceptedSockets[MAX_CLIENTS];
addrinfo* resultingAddress = NULL;
timeval timeVal;

CRITICAL_SECTION CSAnalogData;
CRITICAL_SECTION CSStatusData;
CRITICAL_SECTION CSAnalogSubs;
CRITICAL_SECTION CSStatusSubs;

NODE* publisherList = NULL;
NODE* subscriberList = NULL;
NODE* statusData = NULL;
NODE* analogData = NULL;
NODE* statusSubscribers = NULL;
NODE* analogSubscribers = NULL;
HANDLE listenHandle;


bool InitCriticalSections() {
	InitializeCriticalSection(&CSAnalogData);
	InitializeCriticalSection(&CSStatusData);
	InitializeCriticalSection(&CSAnalogSubs);
	InitializeCriticalSection(&CSStatusSubs);

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

		}
		else if (value == SOCKET_ERROR) {
			printf("select failed with error: %d\n", WSAGetLastError());
			for (int i = 0; i < MAX_CLIENTS; i++) {
				if (FD_ISSET(acceptedSockets[i], &readfds)) {
					closesocket(acceptedSockets[i]);
					acceptedSockets[i] = INVALID_SOCKET;
				}
			}
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

			}
			else { 
				ProcessMessages();
			}

		}
	}

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

void SetAcceptedSocketsInvalid() {
	for (int i = 0; i < MAX_CLIENTS; i++) {
		acceptedSockets[i] = INVALID_SOCKET;
	}
}

bool TCPReceive(SOCKET connectSocket, char* recvbuf, size_t len) {

	int iResult = recv(connectSocket, (char*)recvbuf, len, 0);
	if (!(iResult > 0))
	{
		return false;
	}
	return true;
}

void ProcessMessages() {
	char* data = (char*)malloc(sizeof(Measurement));
	for (int i = 0; i < MAX_CLIENTS; i++) {
		if (FD_ISSET(acceptedSockets[i], &readfds)) {

			bool succes = TCPReceive(acceptedSockets[i], data, sizeof(Measurement));

			if (!succes) {
				EnterCriticalSection(&CSAnalogSubs);
				DeleteNode(&analogSubscribers, &acceptedSockets[i], sizeof(SOCKET));
				LeaveCriticalSection(&CSAnalogSubs);

				EnterCriticalSection(&CSStatusSubs);
				DeleteNode(&statusSubscribers, &acceptedSockets[i], sizeof(SOCKET));
				LeaveCriticalSection(&CSStatusSubs);

				closesocket(acceptedSockets[i]);
				acceptedSockets[i] = INVALID_SOCKET;
				continue;
			}

			SOCKET* ptr = &acceptedSockets[i];
			if (data[0] == 'a') {
				EnterCriticalSection(&CSAnalogSubs);
				GenericListPushAtStart(&analogSubscribers, ptr, sizeof(SOCKET));
				SendToNewSubscriber(acceptedSockets[i], analogData);
				LeaveCriticalSection(&CSAnalogSubs);
			}
			else if (data[0] == 's') {
				EnterCriticalSection(&CSStatusSubs);
				GenericListPushAtStart(&statusSubscribers, ptr, sizeof(SOCKET));
				SendToNewSubscriber(acceptedSockets[i], statusData);
				LeaveCriticalSection(&CSStatusSubs);
			}
			else {
				Measurement* newMeasurement = (Measurement*)malloc(sizeof(Measurement));
				memcpy(newMeasurement, data, sizeof(Measurement));
				ProcessMeasurement(newMeasurement);
				free(newMeasurement);
			}
		}
	}
	free(data);
}

void ProcessMeasurement(Measurement* measure) {
	printf("Primljena poruka: ");
	PrintMeasurement(measure);

	switch (measure->topic)
	{
	case Analog:
		EnterCriticalSection(&CSAnalogData);
		GenericListPushAtStart(&analogData, measure, sizeof(Measurement));
		UpdateSubscribers(measure, analogSubscribers);
		LeaveCriticalSection(&CSAnalogData);
		break;
	case Status:
		EnterCriticalSection(&CSStatusData);
		GenericListPushAtStart(&statusData, measure, sizeof(Measurement));
		UpdateSubscribers(measure, statusSubscribers);
		LeaveCriticalSection(&CSStatusData);
		break;
	default:
		printf("[ERROR] Topic %d not supported.", measure->topic);
		break;
	}
}

void Cleanup() {
	closesocket(listenSocket);
	for (int i = 0; i < MAX_CLIENTS; ++i) {
		closesocket(acceptedSockets[i]);
	}
	WSACleanup();

	FreeGenericList(&publisherList);
	FreeGenericList(&subscriberList);
	FreeGenericList(&statusData);
	FreeGenericList(&analogData);
	FreeGenericList(&statusSubscribers);
	FreeGenericList(&analogSubscribers);

	SAFE_DELETE_HANDLE(listenHandle);

	DeleteCriticalSection(&CSStatusData);
	DeleteCriticalSection(&CSAnalogData);
	DeleteCriticalSection(&CSStatusSubs);
	DeleteCriticalSection(&CSAnalogSubs);

	printf("Service freed all memory.");
	getchar();
}

bool TCPSend(SOCKET connectSocket, Measurement measurment) {
	char* data = (char*)malloc(sizeof(Measurement));
	memcpy(data, (const void*)&measurment, sizeof(Measurement));
	int iResult = send(connectSocket, data, sizeof(Measurement), 0);
	free(data); 
	if (iResult == SOCKET_ERROR)
	{
		printf("send failed with error: %d\n", WSAGetLastError());
		return false;
	}
	return true;
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

void SendToNewSubscriber(SOCKET subscribe, NODE* dataHead) {
	Node* temp = dataHead;
	if (temp == NULL) {
		return;
	}
	Measurement* data = (Measurement*)malloc(sizeof(Measurement));
	while (temp != NULL) {
		memcpy(data, temp->data, sizeof(Measurement));
		TCPSend(subscribe, *data);
		temp = temp->next;
	}
	free(data);
}







