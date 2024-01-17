#pragma once

#define _CRT_NONSTDC_NO_DEPRECATE
#define _CRT_SECURE_NO_WARNINGS
#define WIN32_LEAN_AND_MEAN
#define _WINSOCK_DEPRECATED_NO_WARNINGS

#include <windows.h>
#include <WinSock2.h>
#include <WS2tcpip.h>
#include <stdlib.h>
#include <stdio.h>
#include <conio.h>
#include <time.h>
#include "../Common/Measurement.cpp"
#pragma comment(lib, "ws2_32.lib")

#define WIN32_LEAN_AND_MEAN
#define DEFAULT_BUFLEN 524
#define DEFAULT_PORT 5555
// Initializes WinSock2 library
// Returns true if succeeded, false otherwise.

// socket used to communicate with server
SOCKET connectSocket = INVALID_SOCKET;

int Connect();
bool InitializeWindowsSockets();
void SendChoices(int choice, int interval);
Measurement* GenerateMeasurement();
Measurement* CreateMeasurement();
bool TCPSend(SOCKET connectSocket, Measurement Measurement);

int Connect() {
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

void SendChoices(int choice, int interval) {
	while (true) {
		Measurement* m = (Measurement*)malloc(sizeof(Measurement));
		if (choice == 1) {
			m = CreateMeasurement();
		}
		else m = GenerateMeasurement();

		if (TCPSend(connectSocket, *m)) {
			PrintMeasurement(m);
		}
		else {
			printf("Doslo je do greske prilikom slanja\n");
		}
		free(m);
		Sleep(interval);
	}
}

Measurement* GenerateMeasurement() {
	Measurement* measure = (Measurement*)malloc(sizeof(Measurement));
	measure->value = (rand() % 100) + 1;
	int topic = (rand() % 2);
	int type = (rand() % 2);
	int minus = (rand() % 2);
	switch (topic) {
	case 0:
		measure->topic = Status;
		measure->value = (rand() % 4);
		break;

	case 1:
		measure->topic = Analog;
		measure->type = MER;
		measure->value = (rand() % 100);
		
		if (minus) {
			measure->value *= -1;
		}
		break;
	}
	if (measure->topic == Status) {
		switch (type)
		{
		case 0:
			measure->type = SWG;
			break;
		case 1:
			measure->type = CRB;
			break;
		}
	}
	return measure;
}

Measurement* CreateMeasurement() {
	Measurement* measure = (Measurement*)malloc(sizeof(Measurement));
	int val;
	char topic[20];
	char type[20];
	while (true) {
		printf("Unesite topik\n");
		scanf("%s", topic);

		if (strcmp(topic, "Analog") == 0) {
			measure->topic = Analog;
			measure->type = MER;
			printf("Tip: MER\n");
			break;
		}
		else if (strcmp(topic, "Status") == 0) {
			measure->topic = Status;
			while (true) {
				printf("Unesite tip\n");
				scanf("%s", type);
				if (strcmp(type, "SWG") == 0) {
					measure->type = SWG;
					break;
				}
				else if (strcmp(type, "CRB") == 0) {
					measure->type = CRB;
					break;
				}
				printf("Unesite validan tip za Status topik (SWG, CRB)\n");
			}
			break;
		}
		printf("Unesite validan topik (Analog, Status)\n");
	}

	printf("Unesite vrednost\n");
	scanf("%d", &val);
	measure->value = val;

	return measure;
}

bool TCPSend(SOCKET connectSocket, Measurement Measurement) {
	char* data = (char*)malloc(sizeof(Measurement));
	memcpy(data, (const void*)&Measurement, sizeof(Measurement));
	int iResult = send(connectSocket, data, sizeof(Measurement), 0);
	free(data);
	if (iResult == SOCKET_ERROR)
	{
		printf("send failed with error: %d\n", WSAGetLastError());
		return false;
	}
	return true;
}