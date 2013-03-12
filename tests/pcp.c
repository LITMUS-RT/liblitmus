#include <fcntl.h>
#include <unistd.h>
#include <stdio.h>
#include <sys/wait.h> /* for waitpid() */


#include "tests.h"
#include "litmus.h"


TESTCASE(lock_pcp, P_FP,
	 "PCP acquisition and release")
{
	int fd, od, cpu = 0;

	SYSCALL( fd = open(".pcp_locks", O_RDONLY | O_CREAT, S_IRUSR) );

	SYSCALL( sporadic_partitioned(10, 100, cpu) );
	SYSCALL( task_mode(LITMUS_RT_TASK) );

	SYSCALL( od = open_pcp_sem(fd, 0, cpu) );

	SYSCALL( litmus_lock(od) );
	SYSCALL( litmus_unlock(od) );

	SYSCALL( litmus_lock(od) );
	SYSCALL( litmus_unlock(od) );

	SYSCALL( litmus_lock(od) );
	SYSCALL( litmus_unlock(od) );

	/* tasks may not unlock resources they don't own */
	SYSCALL_FAILS(EINVAL, litmus_unlock(od) );

	SYSCALL( od_close(od) );

	SYSCALL( close(fd) );

	SYSCALL( remove(".pcp_locks") );
}

TESTCASE(pcp_inheritance, P_FP,
	 "PCP priority inheritance")
{
	int fd, od, cpu = 0;

	int child_hi, child_lo, child_middle, status, waiters;
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

	SYSCALL( fd = open(".pcp_locks", O_RDONLY | O_CREAT, S_IRUSR) );


	child_lo = FORK_TASK(
		params.priority = LITMUS_LOWEST_PRIORITY;
		params.phase    = 0;
		SYSCALL( set_rt_task_param(gettid(), &params) );
		SYSCALL( be_migrate_to_cpu(params.cpu) );
		SYSCALL( task_mode(LITMUS_RT_TASK) );

		SYSCALL( od = open_pcp_sem(fd, 0, cpu) );

		SYSCALL( wait_for_ts_release() );

		SYSCALL( litmus_lock(od) );
		start = cputime();
		while (cputime() - start < 0.25)
			;
		SYSCALL( litmus_unlock(od) );

		SYSCALL(sleep_next_period() );
		);

	child_middle = FORK_TASK(
		params.priority	= LITMUS_HIGHEST_PRIORITY + 1;
		params.phase    = ms2lt(100);

		SYSCALL( set_rt_task_param(gettid(), &params) );
		SYSCALL( be_migrate_to_cpu(params.cpu) );
		SYSCALL( task_mode(LITMUS_RT_TASK) );


		SYSCALL( wait_for_ts_release() );

		start = cputime();
		while (cputime() - start < 5)
			;
		SYSCALL( sleep_next_period() );
		);

	child_hi = FORK_TASK(
		params.priority	= LITMUS_HIGHEST_PRIORITY;
		params.phase    = ms2lt(50);

		SYSCALL( set_rt_task_param(gettid(), &params) );
		SYSCALL( be_migrate_to_cpu(params.cpu) );
		SYSCALL( task_mode(LITMUS_RT_TASK) );

		SYSCALL( od = open_pcp_sem(fd, 0, cpu) );

		SYSCALL( wait_for_ts_release() );

		start = wctime();
		/* block on semaphore */
		SYSCALL( litmus_lock(od) );
		SYSCALL( litmus_unlock(od) );
		stop  = wctime();

		/* Assert we had some blocking. */
		ASSERT( stop - start > 0.1);

		/* Assert we woke up 'soonish' after the sleep. */
		ASSERT( stop - start < 1 );

		SYSCALL( kill(child_middle, SIGUSR2) );
		SYSCALL( kill(child_lo, SIGUSR2) );
		);

	do {
		waiters = get_nr_ts_release_waiters();
		ASSERT( waiters >= 0 );
	} while (waiters != 3);

	SYSCALL( be_migrate_to_cpu(1) );

	waiters = release_ts(&delay);

	SYSCALL( waitpid(child_hi, &status, 0) );
	ASSERT( status == 0 );

	SYSCALL( waitpid(child_lo, &status, 0) );
	ASSERT( status ==  SIGUSR2);

	SYSCALL( waitpid(child_middle, &status, 0) );
	ASSERT( status ==  SIGUSR2);
}

TESTCASE(srp_ceiling_blocking, P_FP | PSN_EDF,
	 "SRP ceiling blocking")
{
	int fd, od;

	int child_hi, child_lo, child_middle, status, waiters;
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

	SYSCALL( fd = open(".srp_locks", O_RDONLY | O_CREAT, S_IRUSR) );


	child_lo = FORK_TASK(
		params.priority = LITMUS_LOWEST_PRIORITY;
		params.phase    = 0;
		SYSCALL( set_rt_task_param(gettid(), &params) );
		SYSCALL( be_migrate_to_cpu(params.cpu) );
		SYSCALL( task_mode(LITMUS_RT_TASK) );

		SYSCALL( od = open_srp_sem(fd, 0) );

		SYSCALL( wait_for_ts_release() );

		SYSCALL( litmus_lock(od) );
		start = cputime();
		while (cputime() - start < 0.25)
			;
		SYSCALL( litmus_unlock(od) );
		);

	child_middle = FORK_TASK(
		params.priority	= LITMUS_HIGHEST_PRIORITY + 1;
		params.phase    = ms2lt(100);
		params.relative_deadline -= ms2lt(110);

		SYSCALL( set_rt_task_param(gettid(), &params) );
		SYSCALL( be_migrate_to_cpu(params.cpu) );
		SYSCALL( task_mode(LITMUS_RT_TASK) );


		SYSCALL( wait_for_ts_release() );

		start = cputime();
		while (cputime() - start < 5)
			;
		);

	child_hi = FORK_TASK(
		params.priority	= LITMUS_HIGHEST_PRIORITY;
		params.phase    = ms2lt(50);
		params.relative_deadline -= ms2lt(200);

		SYSCALL( set_rt_task_param(gettid(), &params) );
		SYSCALL( be_migrate_to_cpu(params.cpu) );
		SYSCALL( task_mode(LITMUS_RT_TASK) );

		SYSCALL( od = open_srp_sem(fd, 0) );

		SYSCALL( wait_for_ts_release() );

		start = wctime();
		/* block on semaphore */
		SYSCALL( litmus_lock(od) );
		SYSCALL( litmus_unlock(od) );
		stop  = wctime();

		/* Assert we had "no" blocking (modulo qemu overheads). */
		ASSERT( stop - start < 0.01);

		SYSCALL( kill(child_middle, SIGUSR2) );
		SYSCALL( kill(child_lo, SIGUSR2) );
		);

	do {
		waiters = get_nr_ts_release_waiters();
		ASSERT( waiters >= 0 );
	} while (waiters != 3);

	SYSCALL( be_migrate_to_cpu(1) );

	waiters = release_ts(&delay);

	SYSCALL( waitpid(child_hi, &status, 0) );
	ASSERT( status == 0 );

	SYSCALL( waitpid(child_lo, &status, 0) );
	ASSERT( status ==  SIGUSR2);

	SYSCALL( waitpid(child_middle, &status, 0) );
	ASSERT( status ==  SIGUSR2);
}

TESTCASE(lock_dpcp, P_FP,
	 "DPCP acquisition and release")
{
	int fd, od, cpu = 1;

	SYSCALL( fd = open(".pcp_locks", O_RDONLY | O_CREAT, S_IRUSR) );

	SYSCALL( sporadic_partitioned(10, 100, 0) );
	SYSCALL( task_mode(LITMUS_RT_TASK) );

	SYSCALL( od = open_dpcp_sem(fd, 0, cpu) );

	SYSCALL( litmus_lock(od) );
	SYSCALL( litmus_unlock(od) );

	SYSCALL( litmus_lock(od) );
	SYSCALL( litmus_unlock(od) );

	SYSCALL( litmus_lock(od) );
	SYSCALL( litmus_unlock(od) );

	/* tasks may not unlock resources they don't own */
	SYSCALL_FAILS(EINVAL, litmus_unlock(od) );

	SYSCALL( od_close(od) );

	SYSCALL( close(fd) );

	SYSCALL( remove(".pcp_locks") );
}

TESTCASE(not_lock_pcp_be, P_FP,
	 "don't let best-effort tasks lock (D|M-)PCP semaphores")
{
	int fd, od;

	SYSCALL( fd = open(".pcp_locks", O_RDONLY | O_CREAT, S_IRUSR) );

	/* BE tasks are not even allowed to open a PCP semaphore */
	SYSCALL_FAILS(EPERM, od = open_pcp_sem(fd, 0, 1) );

	/* BE tasks are not allowed to open a D-PCP semaphore */
	SYSCALL_FAILS(EPERM, od = open_dpcp_sem(fd, 0, 1) );

	/* BE tasks are not allowed to open an M-PCP semaphore */
	SYSCALL_FAILS(EPERM, od = open_mpcp_sem(fd, 0) );

	SYSCALL( close(fd) );

	SYSCALL( remove(".pcp_locks") );

}

TESTCASE(lock_mpcp, P_FP,
	 "MPCP acquisition and release")
{
	int fd, od;

	SYSCALL( fd = open(".pcp_locks", O_RDONLY | O_CREAT, S_IRUSR) );

	SYSCALL( sporadic_partitioned(10, 100, 0) );
	SYSCALL( task_mode(LITMUS_RT_TASK) );

	SYSCALL( od = open_mpcp_sem(fd, 0) );

	SYSCALL( litmus_lock(od) );
	SYSCALL( litmus_unlock(od) );

	SYSCALL( litmus_lock(od) );
	SYSCALL( litmus_unlock(od) );

	SYSCALL( litmus_lock(od) );
	SYSCALL( litmus_unlock(od) );

	/* tasks may not unlock resources they don't own */
	SYSCALL_FAILS(EINVAL, litmus_unlock(od) );

	SYSCALL( od_close(od) );

	SYSCALL( close(fd) );

	SYSCALL( remove(".pcp_locks") );
}
