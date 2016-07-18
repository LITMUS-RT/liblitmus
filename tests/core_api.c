#include <sys/wait.h> /* for waitpid() */
#include <unistd.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <sched.h>

#include "tests.h"
#include "litmus.h"
#include "migration.h"


TESTCASE(set_rt_task_param_invalid_pointer, ALL,
	 "reject invalid rt_task pointers")
{
	SYSCALL_FAILS( EINVAL, set_rt_task_param(gettid(), NULL));

	SYSCALL_FAILS( EFAULT, set_rt_task_param(gettid(), (void*) 0x123 ));
}

TESTCASE(get_set_rt_task_param, ALL,
	 "read back rt_task values")
{
	struct rt_task params;
	int cpu = num_online_cpus() - 1;
	init_rt_task_param(&params);
	params.cpu        = cpu;
	params.exec_cost  = 90;
	params.period     = 321;
	params.relative_deadline = 123;

	/* set params */
	SYSCALL( set_rt_task_param(gettid(), &params) );

	/* now clear our copy */
	memset(&params, 0, sizeof(params));

	/* get params */
	SYSCALL( get_rt_task_param(gettid(), &params) );

	ASSERT(params.cpu == cpu);
	ASSERT(params.exec_cost == 90);
	ASSERT(params.period == 321);
	ASSERT(params.relative_deadline == 123);
}


TESTCASE(set_rt_task_param_invalid_params, ALL,
	 "reject invalid rt_task values")
{
	struct rt_task params;
	init_rt_task_param(&params);
	params.cpu        = 0;
	params.period     = 100;
	params.relative_deadline = params.period;
	params.phase      = 0;
	params.priority	  = LITMUS_LOWEST_PRIORITY;
	params.cls        = RT_CLASS_HARD;
	params.budget_policy = NO_ENFORCEMENT;

	/* over utilize */
	params.exec_cost  = 110;
	SYSCALL_FAILS( EINVAL, set_rt_task_param(gettid(), &params) );
	params.exec_cost = 90;

	/* infeasible density */
	params.cpu  = 0;
	params.relative_deadline = 30;
	SYSCALL_FAILS( EINVAL, set_rt_task_param(gettid(), &params) );

	/* bad task */
	params.relative_deadline = params.period;
	SYSCALL_FAILS( EINVAL, set_rt_task_param(-1, &params) );


	/* now try correct params */
	SYSCALL( set_rt_task_param(gettid(), &params) );
}

TESTCASE(reject_bad_priorities, P_FP,
	 "reject invalid priorities")
{
	struct rt_task params;
	init_rt_task_param(&params);
	params.cpu        = 0;
	params.exec_cost  =  10;
	params.period     = 100;
	params.relative_deadline = params.period;
	params.phase      = 0;
	params.cls        = RT_CLASS_HARD;
	params.budget_policy = NO_ENFORCEMENT;

	SYSCALL( be_migrate_to_cpu(params.cpu) );

	/* too high */
	params.priority	  = 0;
	SYSCALL( set_rt_task_param(gettid(), &params) );
	SYSCALL_FAILS( EINVAL, task_mode(LITMUS_RT_TASK) );

	/* too low */
	params.priority   = LITMUS_MAX_PRIORITY;
	SYSCALL( set_rt_task_param(gettid(), &params) );
	SYSCALL_FAILS( EINVAL, task_mode(LITMUS_RT_TASK) );

}

TESTCASE(accept_valid_priorities, P_FP,
	 "accept lowest and highest valid priorities")
{
	struct rt_task params;
	init_rt_task_param(&params);
	params.cpu        = 0;
	params.exec_cost  =  10;
	params.period     = 100;
	params.relative_deadline = params.period;
	params.phase      = 0;
	params.cls        = RT_CLASS_HARD;
	params.budget_policy = NO_ENFORCEMENT;

	SYSCALL( be_migrate_to_cpu(params.cpu) );

	/* acceptable */
	params.priority   = LITMUS_LOWEST_PRIORITY;
	SYSCALL( set_rt_task_param(gettid(), &params) );
	SYSCALL( task_mode(LITMUS_RT_TASK) );
	SYSCALL( task_mode(BACKGROUND_TASK) );

	params.priority   = LITMUS_HIGHEST_PRIORITY;
	SYSCALL( set_rt_task_param(gettid(), &params) );
	SYSCALL( task_mode(LITMUS_RT_TASK) );
	SYSCALL( task_mode(BACKGROUND_TASK) );
}

TESTCASE(job_control_non_rt, ALL,
	 "reject job control for non-rt tasks")
{
	SYSCALL_FAILS( EINVAL, sleep_next_period() );

	SYSCALL_FAILS( EINVAL, wait_for_job_release(0) );
}


TESTCASE(rt_fork_non_rt, LITMUS,
	 "children of RT tasks are not automatically RT tasks")
{
	unsigned int pid, job_no;
	int status;

	SYSCALL( sporadic_partitioned(ms2ns(10), ms2ns(100), 0) );
	SYSCALL( task_mode(LITMUS_RT_TASK) );

	pid = fork();

	ASSERT( pid != -1 );

	if (pid == 0) {
		/* child */

		SYSCALL_FAILS( EINVAL, sleep_next_period() );
		SYSCALL_FAILS( EINVAL, wait_for_job_release(0) );
		SYSCALL_FAILS( EPERM, get_job_no(&job_no) );

		exit(0);
	} else {
		/* parent */

		SYSCALL( sleep_next_period() );
		SYSCALL( wait_for_job_release(3) );
		SYSCALL( get_job_no(&job_no) );

		SYSCALL( task_mode(BACKGROUND_TASK) );

		SYSCALL( waitpid(pid, &status, 0) );

		ASSERT(WEXITSTATUS(status) == 0);
	}
}

TESTCASE(ctrl_page_writable, ALL,
	 "tasks have write access to /dev/litmus/ctrl mappings")
{
	volatile int *ctrl_page = (volatile int*) get_ctrl_page();

	/* init_litmus() should have mapped the page already  */
	ASSERT(ctrl_page != NULL);

	/* These should work without page faults. */
	enter_np();
	exit_np();

	/* Try poking the memory directly. */

	ctrl_page[32] = 0x12345678;
}

TESTCASE(set_cpu_mapping, LITMUS,
	 "task's CPU affinity is set to CPU set that is read from file")
{
	char buf[4096/4	/* enough chars for hex data (4 CPUs per char) */
		+ 4096/(4*8) /* for commas (separate groups of 8 chars) */
		+ 1] = {0}; /* for \0 */
	int len;
	cpu_set_t *set;
	size_t sz;
	
	/*set affinity to CPU 0 */
	strcpy(buf, "20");
	len = strnlen(buf, sizeof(buf));
	set_mapping(buf, len, &set, &sz);
	ASSERT( CPU_ISSET_S(5, sz, set) );

	/*set affinity to CPU 29 */
	strcpy(buf, "20000000");
	len = strnlen(buf, sizeof(buf));
	set_mapping(buf, len, &set, &sz);
	ASSERT( CPU_ISSET_S(29, sz, set) );

	/*set affinity to CPUS 27 and 39 */
	strcpy(buf, "80,08000000");
	len = strnlen(buf, sizeof(buf));
	set_mapping(buf, len, &set, &sz);
	ASSERT( CPU_ISSET_S(27, sz, set) );
	ASSERT( CPU_ISSET_S(39, sz, set) );

	/*set affinity to CPUS 96 */
	strcpy(buf, "1,00000000,00000000,00000000");
	len = strnlen(buf, sizeof(buf));
	set_mapping(buf, len, &set, &sz);
	ASSERT( CPU_ISSET_S(96, sz, set) );

}



TESTCASE(suspended_admission, LITMUS,
	 "admission control handles suspended tasks correctly")
{
	int child_rt,  status;
	struct sched_param param;

	int pipefd[2], err, token = 0;
	struct rt_task params;

        SYSCALL( pipe(pipefd) );

	child_rt = FORK_TASK(
		child_rt  = gettid();
		/* make sure we are on the right CPU */
		be_migrate_to_cpu(0);

		/* carry out a blocking read to suspend */
		err = read(pipefd[0], &token, sizeof(token));
		ASSERT(err = sizeof(token));
		ASSERT(token == 1234);

		/* when we wake up, we should be a real-time task */
		ASSERT( sched_getscheduler(child_rt) == SCHED_LITMUS );

		SYSCALL( sleep_next_period() );
		SYSCALL( sleep_next_period() );

		exit(0);
		);

	/* give child some time to suspend */
	SYSCALL( lt_sleep(ms2ns(100)) );

	init_rt_task_param(&params);
	params.cpu        = 0;
	params.exec_cost  = ms2ns(10);
	params.period     = ms2ns(100);

	/* configure parameters of child */
	SYSCALL( set_rt_task_param(child_rt, &params) );

	/* make it a real-time task */
	param.sched_priority = 0;
	SYSCALL( sched_setscheduler(child_rt, SCHED_LITMUS, &param) );

	/* should be a real-time task now */
	ASSERT( sched_getscheduler(child_rt) == SCHED_LITMUS );

	/* unblock it */
	token = 1234;
	ASSERT( write(pipefd[1], &token, sizeof(token)) == sizeof(token) );

	/* wait for child to exit */
	SYSCALL( waitpid(child_rt, &status, 0) );
	ASSERT( status == 0 );
}

TESTCASE(running_admission, LITMUS,
	 "admission control handles running tasks correctly")
{
	int child_rt,  status;
	struct sched_param param;

	int pipefd[2], err, token = 0, scheduler;
	struct rt_task params;

        SYSCALL( pipe(pipefd) );

	child_rt = FORK_TASK(
		child_rt  = gettid();
		/* make sure we are on the right CPU */
		be_migrate_to_cpu(0);

		/* unblock parent */
		token = 1234;
		ASSERT( write(pipefd[1], &token, sizeof(token)) == sizeof(token) );

		scheduler = -1;

		while (scheduler != SCHED_LITMUS) {
			SYSCALL( scheduler = sched_getscheduler(child_rt) );
		}

		SYSCALL( sleep_next_period() );
		SYSCALL( sleep_next_period() );

		exit(0);
		);

	/* carry out a blocking read to suspend */
	err = read(pipefd[0], &token, sizeof(token));
	ASSERT(err = sizeof(token));
	ASSERT(token == 1234);

	/* give child some time to run */
	SYSCALL( lt_sleep(ms2ns(10)) );

	init_rt_task_param(&params);
	params.cpu        = 0;
	params.exec_cost  = ms2ns(10);
	params.period     = ms2ns(100);

	/* configure parameters of child */
	SYSCALL( set_rt_task_param(child_rt, &params) );

	/* make it a real-time task */
	param.sched_priority = 0;
	SYSCALL( sched_setscheduler(child_rt, SCHED_LITMUS, &param) );

	/* should be a real-time task now */
	ASSERT( sched_getscheduler(child_rt) == SCHED_LITMUS );

	/* wait for child to exit */
	SYSCALL( waitpid(child_rt, &status, 0) );
	ASSERT( status == 0 );
}
