#include "../PubSubEngine/PubSubEngine.h"


int  main()
{
	DWORD listenID;
	DWORD workerID;
	

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
	workerManagerHandle = CreateThread(NULL, 0, &InitWorkerThreads, (LPVOID)0, 0, &workerID);

	printf("Service running, press 'q' for shutdown\n");
	while (true) {
		char key = getchar();
		if (key == 'q') {
			Cleanup();
			break;
		}
	}
	return 0;
}
