#include <stdio.h>

#include <sys/time.h>
#include <time.h>

#include "litmus.h"

/* CPU time consumed so far in seconds */
double cputime(void)
{
	struct timespec ts;
	int err;
	err = clock_gettime(CLOCK_THREAD_CPUTIME_ID, &ts);
	if (err != 0)
		perror("clock_gettime");
	return (ts.tv_sec + 1E-9 * ts.tv_nsec);
}

/* wall-clock time in seconds */
double wctime(void)
{
	struct timeval tv;
	gettimeofday(&tv, NULL);
	return (tv.tv_sec + 1E-6 * tv.tv_usec);
}

/* current time, according to CLOCK_MONOTONIC */
double monotime(void)
{
	struct timespec ts;
	int err;
	err = clock_gettime(CLOCK_MONOTONIC, &ts);
	if (err != 0)
		perror("clock_gettime");
	return (ts.tv_sec + 1E-9 * ts.tv_nsec);
}

int lt_sleep(lt_t timeout)
{
	struct timespec delay;

	delay.tv_sec  = timeout / 1000000000L;
	delay.tv_nsec = timeout % 1000000000L;
	return nanosleep(&delay, NULL);
}
