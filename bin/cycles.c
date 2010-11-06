#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

#include "asm/cycles.h"

int main(int argc, char** argv)
{
	cycles_t t1, t2;
	int secs = 1;

	if (argc > 1) {
		secs = atoi(argv[1]);
		if (secs <= 0)
			secs = 1;
	}
	while (1) {
		t1 = get_cycles();
		sleep(secs);
		t2 = get_cycles();
		t2 -= t1;
		printf("%.2f/sec  %.2f/msec  %.2f/usec\n",
		       t2 / (double) secs,
		       t2 / (secs * 1000.0),
		       t2 / (secs * 1000000.0));
	}
	return 0;
}
