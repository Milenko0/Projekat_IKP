#include "../Publisher/Publisher.h"


int main()
{
	// variable used to store function return value
	int iResult;
	srand(time(NULL));

	if (InitializeWindowsSockets() == false)
	{
		return 1;
	}

	if (Connect())
		return 1;

	printf("Publisher pokrenut.");
	
	int interval = 500;
	int choice;
	int pom=0;
	while (true)
	{
		printf(" Izaberite nacin izrade poruke:\n1)Rucno\n2)Nasumicno\n");
		scanf("%d", &choice);
		switch (choice)
		{
		case 1:
			break;
		case 2:
			printf("Izaberite interval slanja nasumicnih poruka u ms:\n");
			scanf("%d", &interval);
			break;
		default:
			printf("Niste izabrali validnu opcuju.\n");
			pom = 1;
			break;
		}
		if (!pom) break;
		pom = 0;
		
	}

	SendChoices(choice, interval);

	// cleanup
	closesocket(connectSocket);
	WSACleanup();
	return 0;
}


