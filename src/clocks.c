#include <stdio.h>

#include <sys/time.h>
#include <errno.h>
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

/* Current time used by the LITMUS^RT scheduler.
 * This is just CLOCK_MONOTONIC and hence the same
 * as monotime(), but the result is given in nanoseconds
 * as a value of type lt_t.*/
lt_t litmus_clock(void)
{
	struct timespec ts;
	int err;
	err = clock_gettime(CLOCK_MONOTONIC, &ts);
	if (err != 0)
		perror("clock_gettime");
	return ((lt_t) s2ns(ts.tv_sec)) + (lt_t) ts.tv_nsec;
}

static void do_sleep_until(struct timespec *ts, clockid_t clock_id)
{
	int err;

	do {
		/* sleep to exact absolute release time */
		err = clock_nanosleep(clock_id, TIMER_ABSTIME, ts, NULL);
		/* If we were interrupted by a signal and we didn't terminate,
		 * then keep sleeping. */
	} while (err != 0 && errno == EINTR);
}

/* Sleep until we've reached wake_up_time (in seconds) on the given timeline */
static void clock_sleep_until(double wake_up_time, clockid_t clock_id)
{
	struct timespec ts;

	/* convert from double (seconds) */
	ts.tv_sec = (time_t) wake_up_time;
	ts.tv_nsec = (wake_up_time - ts.tv_sec) * 1E9;

	do_sleep_until(&ts, clock_id);
}

/* Sleep until we've reached wake_up_time (in seconds) on the CLOCK_MONOTONIC
 * timeline. */
void sleep_until_mono(double wake_up_time)
{
	clock_sleep_until(wake_up_time, CLOCK_MONOTONIC);
}

/* Sleep until we've reached wake_up_time (in seconds) on the CLOCK_MONOTONIC
 * timeline. */
void sleep_until_wc(double wake_up_time)
{
	clock_sleep_until(wake_up_time, CLOCK_REALTIME);
}

/* Sleep until we've reached wake_up_time (in nanoseconds) on the
 * CLOCK_MONOTONIC  timeline. */
void lt_sleep_until(lt_t wake_up_time)
{
	struct timespec ts;

	/* convert from double (seconds) */
	ts.tv_sec = (time_t) ns2s(wake_up_time);
	ts.tv_nsec = (long) (wake_up_time - s2ns(ts.tv_sec));

	do_sleep_until(&ts, CLOCK_MONOTONIC);
}

int lt_sleep(lt_t timeout)
{
	struct timespec delay;

	delay.tv_sec  = timeout / 1000000000L;
	delay.tv_nsec = timeout % 1000000000L;
	return nanosleep(&delay, NULL);
}
