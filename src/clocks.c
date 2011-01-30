#include <stdio.h>

#include <sys/time.h>
#include <time.h>

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
