#include "../Publisher/Publisher.h"

int __cdecl main(int argc, char** argv)
{
	// variable used to store function return value
	int iResult;


	if (InitializeWindowsSockets() == false)
	{
		return 1;
	}

	if (Connect())
		return 1;

	// cleanup
	closesocket(connectSocket);
	WSACleanup();

	return 0;
}