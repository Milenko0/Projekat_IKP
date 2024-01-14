#include "../Common/Measurement.h";
#include <stdio.h>

void PrintMeasurement(Measurement* m) {
	const char* topics[] = { "Analog", "Status" };
	const char* types[] = { "SWG", "CRB", "MER" };
	printf("Mera: ");
	printf(" %s %s %d\n", topics[m->topic], types[m->type], m->value);
}