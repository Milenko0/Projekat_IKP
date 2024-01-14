#include "../PubSubEngine/PubSubEngine.h"


int  main()
{
	DWORD listenID;
	// Socket used for listening for new clients 
	SOCKET listenSocket1 = INVALID_SOCKET;
	int iResultPublisher, IResultSubscriber;

	if (InitializeWindowsSockets() == false)
	{
		return 1;
	}

	listenSocket1 = InitializeListenSocket();
	if (listenSocket1 == SOCKET_ERROR || listenSocket1 == INVALID_SOCKET) return 1;



	InitCriticalSections();
	SetAcceptedSocketsInvalid();

	printf("Server initialized, waiting for clients.\n");
	listenHandle = CreateThread(NULL, 0, &Listen, (LPVOID)0, 0, &listenID);

	if (listenHandle) {
		WaitForSingleObject(listenHandle, INFINITE);
	}
	Cleanup();
	//WSACleanup();

	return 0;
}
