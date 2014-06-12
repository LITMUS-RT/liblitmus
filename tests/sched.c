#include <sys/wait.h> /* for waitpid() */
#include <unistd.h>
#include <stdio.h>
#include <math.h>

#include "tests.h"
#include "litmus.h"

TESTCASE(preempt_on_resume, P_FP | PSN_EDF,
	 "preempt lower-priority task when a higher-priority task resumes")
{
	int child_hi, child_lo, status, waiters;
	lt_t delay = ms2ns(100);
	double start, stop;

	struct rt_task params;
	init_rt_task_param(&params);
	params.cpu        = 0;
	params.exec_cost  = ms2ns(10000);
	params.period     = ms2ns(100000);

	child_lo = FORK_TASK(
		params.priority = LITMUS_LOWEST_PRIORITY;
		SYSCALL( set_rt_task_param(gettid(), &params) );
		SYSCALL( be_migrate_to_cpu(params.cpu) );
		SYSCALL( task_mode(LITMUS_RT_TASK) );

		SYSCALL( wait_for_ts_release() );

		start = cputime();

		while (cputime() - start < 10)
			;

		);

	child_hi = FORK_TASK(
		params.priority	= LITMUS_HIGHEST_PRIORITY;
		params.relative_deadline -= 1000000;
		SYSCALL( set_rt_task_param(gettid(), &params) );
		SYSCALL( be_migrate_to_cpu(params.cpu) );
		SYSCALL( task_mode(LITMUS_RT_TASK) );

		SYSCALL( wait_for_ts_release() );

		start = cputime();

		while (cputime() - start < 0.1)
			;

		start = wctime();
		SYSCALL( lt_sleep(ms2ns(100)) );
		stop = wctime();

		SYSCALL( kill(child_lo, SIGUSR2) );

		if (stop - start >= 0.2)
			fprintf(stderr, "\nHi-prio delay = %fsec\n",
				stop - start - (ms2ns(100) / (float)s2ns(1)));

		/* Assert we woke up 'soonish' after the sleep. */
		ASSERT( stop - start < 0.2 );
		);


	do {
		waiters = get_nr_ts_release_waiters();
		ASSERT( waiters >= 0 );
	} while (waiters != 2);

	waiters = release_ts(&delay);

	SYSCALL( waitpid(child_hi, &status, 0) );
	ASSERT( status == 0 );

	SYSCALL( waitpid(child_lo, &status, 0) );
	ASSERT( status ==  SIGUSR2);
}

#define PERIOD 100 /* in ms */
#define JOBS 10
#define ACCEPTABLE_MARGIN 5 /* in ms --- must be largish for QEMU */

TESTCASE(jobs_are_rate_limited, LITMUS,
	 "periodic jobs are rate-limited (w/o synchronous release)")
{
	struct rt_task params;
	double start, end, actual_delta_ms;
	int i;

	init_rt_task_param(&params);
	params.cpu        = 0;
	params.exec_cost  = ms2ns(40);
	params.period     = ms2ns(PERIOD);
	params.release_policy = TASK_PERIODIC;

	SYSCALL( set_rt_task_param(gettid(), &params) );
	SYSCALL( be_migrate_to_cpu(params.cpu) );
	SYSCALL( task_mode(LITMUS_RT_TASK) );

	start = wctime();
	for (i = 0; i < JOBS; i++)
		SYSCALL( sleep_next_period() );
	end  = wctime();

	SYSCALL( task_mode(BACKGROUND_TASK) );

	actual_delta_ms = (end - start) * 1000.0;
	if (fabs(JOBS * PERIOD - actual_delta_ms) > ACCEPTABLE_MARGIN)
		fprintf(stderr, "actual_delta_ms:%.4f  expected:%.4f\n",
			actual_delta_ms, (double) (JOBS * PERIOD));
	ASSERT( fabs(JOBS * PERIOD - actual_delta_ms) <= ACCEPTABLE_MARGIN);
}

TESTCASE(jobs_are_rate_limited_synch, LITMUS,
	 "periodic jobs are rate-limited (w/ synchronous release)")
{
	struct rt_task params;
	double start, end, actual_delta_ms;
	int i;
	int child, status, waiters;
	lt_t delay = ms2ns(100);

	child = FORK_TASK(
		init_rt_task_param(&params);
		params.cpu        = 0;
		params.exec_cost  = ms2ns(5);
		params.period     = ms2ns(PERIOD);
		params.release_policy = TASK_PERIODIC;

		SYSCALL( set_rt_task_param(gettid(), &params) );
		SYSCALL( be_migrate_to_cpu(params.cpu) );
		SYSCALL( task_mode(LITMUS_RT_TASK) );

		SYSCALL(  wait_for_ts_release() );

		start = wctime();
		for (i = 0; i < JOBS; i++)
			SYSCALL( sleep_next_period() );
		end  = wctime();

		SYSCALL( task_mode(BACKGROUND_TASK) );

		actual_delta_ms = (end - start) * 1000.0;
		if (fabs(JOBS * PERIOD - actual_delta_ms) > ACCEPTABLE_MARGIN)
			fprintf(stderr, "actual_delta_ms:%.4f  expected:%.4f\n",
				actual_delta_ms, (double) (JOBS * PERIOD));
		ASSERT( fabs(JOBS * PERIOD - actual_delta_ms) <= ACCEPTABLE_MARGIN);
	);

	do {
		waiters = get_nr_ts_release_waiters();
		ASSERT( waiters >= 0 );
	} while (waiters != 1);

	waiters = release_ts(&delay);


	/* wait for child to exit */
	SYSCALL( waitpid(child, &status, 0) );
	ASSERT( status == 0 );
}
