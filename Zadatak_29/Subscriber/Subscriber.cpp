#include "../Subscriber/Subscriber.h"


int main()
{
	if (InitializeWindowsSockets() == false)
	{
		return 1;
	}

	if (Connect())
		return 1;

    printf("Subscriber pokrenut.\n");

    Subscribe();

    printf("Subscriber je pretplacen na izabrani topik.\n");

    StartRecieveThread();

    getchar();
	// cleanup
	closesocket(connectSocket);
	WSACleanup();

	return 0;
}

