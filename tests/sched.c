#include <sys/wait.h> /* for waitpid() */
#include <unistd.h>
#include <stdio.h>

#include "tests.h"
#include "litmus.h"

TESTCASE(preempt_on_resume, P_FP | PSN_EDF,
	 "preempt lower-priority task when a higher-priority task resumes")
{
	int child_hi, child_lo, status, waiters;
	lt_t delay = ms2lt(100);
	double start, stop;

	struct rt_task params;
	params.cpu        = 0;
	params.exec_cost  =  ms2lt(10000);
	params.period     = ms2lt(100000);
	params.relative_deadline = params.period;
	params.phase      = 0;
	params.cls        = RT_CLASS_HARD;
	params.budget_policy = NO_ENFORCEMENT;

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
		SYSCALL( lt_sleep(ms2lt(100)) );
		stop = wctime();

		SYSCALL( kill(child_lo, SIGUSR2) );

		if (stop - start >= 0.2)
			fprintf(stderr, "\nHi-prio delay = %fsec\n",
				stop - start - (ms2lt(100) / 1E9));

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


