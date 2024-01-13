
#include <conio.h>
#include "../PubSubEngine/PubSubEngine.h"


int  main(void)
{
	// Socket used for listening for new clients 
	SOCKET listenSocketPublisher = INVALID_SOCKET;
	SOCKET listenSocketSubscriber = INVALID_SOCKET;
	int iResultPublisher, IResultSubscriber;

	if (InitializeWindowsSockets() == false)
	{
		return 1;
	}

	listenSocketPublisher = InitializeListenSocket(PUBLISHER_PORT);
	if (listenSocketPublisher == SOCKET_ERROR || listenSocketPublisher == INVALID_SOCKET) return 1;
	listenSocketSubscriber = InitializeListenSocket(SUBSCRIBER_PORT);
	if (listenSocketSubscriber == SOCKET_ERROR || listenSocketSubscriber == INVALID_SOCKET) return 1;

	// Set listenSocket in listening mode
	iResultPublisher = listen(listenSocketPublisher, SOMAXCONN);
	IResultSubscriber = listen(listenSocketSubscriber, SOMAXCONN);

	if (iResultPublisher == SOCKET_ERROR || IResultSubscriber == SOCKET_ERROR)
	{
		printf("listen failed with error: %d\n", WSAGetLastError());
		closesocket(listenSocketPublisher);
		closesocket(listenSocketSubscriber);
		WSACleanup();
		return 1;
	}

	printf("Server initialized, waiting for clients.\n");

	// cleanup
	closesocket(listenSocketPublisher);
	closesocket(listenSocketSubscriber);
	listenSocketPublisher = INVALID_SOCKET;
	listenSocketSubscriber = INVALID_SOCKET;
	WSACleanup();

	return 0;
}
