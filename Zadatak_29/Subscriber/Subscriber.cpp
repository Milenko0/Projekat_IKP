#include "../Subscriber/Subscriber.h"

int __cdecl main(int argc, char** argv)
{
	if (InitializeWindowsSockets() == false)
	{
		// we won't log anything since it will be logged
		// by InitializeWindowsSockets() function
		return 1;
	}

	//konekcija na server nije uspela
	if (Connect())
		return 1;

	// cleanup
	closesocket(connectSocket);
	WSACleanup();

	return 0;
}
