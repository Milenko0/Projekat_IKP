#pragma once
#define _CRT_NONSTDC_NO_DEPRECATE
#define _CRT_SECURE_NO_WARNINGS
#define WIN32_LEAN_AND_MEAN
#define _WINSOCK_DEPRECATED_NO_WARNINGS
#include <windows.h>
#include <winsock2.h>
#include <ws2tcpip.h>
#include <stdlib.h>
#include <stdio.h>
#include <conio.h>
#include "../Common/Measurement.h";
#include "../Common/Measurement.cpp";
#pragma comment(lib, "ws2_32.lib")

#define WIN32_LEAN_AND_MEAN
#define DEFAULT_BUFLEN 524
#define DEFAULT_PORT 5555

// Initializes WinSock2 library
// Returns true if succeeded, false otherwise.
DWORD WINAPI Receive(LPVOID param);
int Connect();
bool InitializeWindowsSockets();
void Subscribe();
void StartRecieveThread();
bool TCPReceive(SOCKET connectSocket, char* recvbuf, size_t len);
bool TCPSend(SOCKET connectSocket, char* key);
bool Validate(Measurement* measure);

SOCKET connectSocket = INVALID_SOCKET;

DWORD WINAPI Receive(LPVOID param) 
{
	char* data = (char*)malloc(sizeof(Measurement));
	while (true) {
		TCPReceive(connectSocket, data, sizeof(Measurement));
		Measurement* newMeasurement = (Measurement*)malloc(sizeof(Measurement));
		memcpy(newMeasurement, data, sizeof(Measurement));
		if (Validate(newMeasurement)) {
			printf("VALIDNO:\t");
		}
		else {
			printf("NEVALIDNO:\t");
		}
        PrintMeasurement(newMeasurement);
		free(newMeasurement);
		Sleep(10);
	}
}

// socket used to communicate with server

int Connect() 
{
	// create a socket
	connectSocket = socket(AF_INET,
		SOCK_STREAM,
		IPPROTO_TCP);

	if (connectSocket == INVALID_SOCKET)
	{
		printf("socket failed with error: %ld\n", WSAGetLastError());
		WSACleanup();
        return 1;
	}
	// create and initialize address structure
	sockaddr_in serverAddress;
	serverAddress.sin_family = AF_INET;
	serverAddress.sin_addr.s_addr = inet_addr("127.0.0.1");
	serverAddress.sin_port = htons(DEFAULT_PORT);
	// connect to server specified in serverAddress and socket connectSocket
	if (connect(connectSocket, (SOCKADDR*)&serverAddress, sizeof(serverAddress)) == SOCKET_ERROR)
	{
		printf("Unable to connect to server.\n");
		closesocket(connectSocket);
		WSACleanup();
		return 1;
	}
	return 0;
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

bool Validate(Measurement* measure) 
{
    if (measure->topic == Analog && measure->value >= 0) {
        return true;
    }
    else if (measure->topic == Status && (measure->value == 0 || measure->value == 1)) {
        return true;
    }
    return false;
}

void StartRecieveThread() 
{
    DWORD id, param = 1;
    HANDLE handle;
    handle = CreateThread(
        NULL, // default security attributes
        0, // use default stack size
        Receive, // thread function
        &param, // argument to thread function
        0, // use default creation flags
        &id); // returns the thread identifier
    int liI = _getch();
    CloseHandle(handle);
}

void Subscribe() 
{
    printf("Izaberite:\n1) Status\n2) Analog\n3) Status i Analog\n");
    char c = getchar();
    char t1[2] = "s";
    char t2[2] = "a";
    switch (c)
    {
    case '1':
        TCPSend(connectSocket, t1);
        break;
    case '2':
        TCPSend(connectSocket, t2);
        break;
    case '3':
        TCPSend(connectSocket, t1);
        TCPSend(connectSocket, t2);
        break;
    default:
        printf("\nNevalidan unos.\n");
        Subscribe();
    }
}


bool TCPSend(SOCKET connectSocket, char* key)
{
    int iResult = send(connectSocket, (const char*)key, sizeof(char), 0);
    if (iResult == SOCKET_ERROR)
    {
        printf("send failed with error: %d\n", WSAGetLastError());
        return false;
    }
    return true;
}

bool TCPReceive(SOCKET connectSocket, char* recvbuf, size_t len) 
{
    int iResult = recv(connectSocket, (char*)recvbuf, len, 0);
    if (!(iResult > 0))
    {
        return false;
    }
    return true;
}

