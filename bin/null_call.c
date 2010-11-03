#include <sys/time.h>
#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <math.h>

#include "litmus.h"

static void time_null_call(void)
{
	cycles_t t0, t1, t2;
	int ret;

	t0 = get_cycles();
	ret = null_call(&t1);
	t2 = get_cycles();
	if (ret != 0)
		perror("null_call");
	printf("%10" CYCLES_FMT ", " 
	       "%10" CYCLES_FMT ", "
	       "%10" CYCLES_FMT ", "
	       "%10" CYCLES_FMT ", "
	       "%10" CYCLES_FMT ", "
	       "%10" CYCLES_FMT "\n",
	       t0, t1, t2, t1 - t0, t2 - t1, t2 - t0);
}

static struct timespec sec2timespec(double seconds)
{
	struct timespec tspec;
	double full_secs = floor(seconds);
	tspec.tv_sec  = (time_t) full_secs;
	tspec.tv_nsec = (long) ((seconds  - full_secs) * 1000000000L);
	return tspec;
}

int main(int argc, char **argv)
{
	double delay;
	struct timespec sleep_time;
	
	if (argc == 2) {
		delay = atof(argv[1]);
		sleep_time = sec2timespec(delay);
		if (delay <= 0.0)
			fprintf(stderr, "Invalid time spec: %s\n", argv[1]);
		fprintf(stderr, "Measuring syscall overhead every "
			"%lus and %luns.\n",
			(unsigned long) sleep_time.tv_sec,
			(unsigned long) sleep_time.tv_nsec);
		fprintf(stderr, "%10s, %10s, %10s, %10s, %10s, %10s\n",
			"pre", "in kernel", "post", "entry", "exit", "total");
		do {
			time_null_call();
		} while (nanosleep(&sleep_time, NULL) == 0);
	} else {
		fprintf(stderr, "Press enter key to measure...\n");
		while (getchar() != EOF) {
			time_null_call();
		}
	}
	return 0;
}
