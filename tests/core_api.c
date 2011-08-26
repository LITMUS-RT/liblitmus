#include <sys/wait.h> /* for waitpid() */
#include <unistd.h>
#include <stdio.h>

#include "tests.h"
#include "litmus.h"


TESTCASE(set_rt_task_param_invalid_pointer, ALL,
	 "reject invalid rt_task pointers")
{
	SYSCALL_FAILS( EINVAL, set_rt_task_param(gettid(), NULL));

	SYSCALL_FAILS( EFAULT, set_rt_task_param(gettid(), (void*) 0x123 ));
}

TESTCASE(set_rt_task_param_invalid_params, ALL,
	 "reject invalid rt_task values")
{
	struct rt_task params;
	params.cpu        = 0;
	params.period     = 100;
	params.phase      = 0;
	params.cls        = RT_CLASS_HARD;
	params.budget_policy = NO_ENFORCEMENT;

	/* over utilize */
	params.exec_cost  = 110;
	SYSCALL_FAILS( EINVAL, set_rt_task_param(gettid(), &params) );

	/* bad CPU */
	params.exec_cost = 90;
	params.cpu       = -1;
	SYSCALL_FAILS( EINVAL, set_rt_task_param(gettid(), &params) );

	/* bad task */
	params.cpu  = 0;
	SYSCALL_FAILS( EINVAL, set_rt_task_param(-1, &params) );


	/* now try correct params */
	SYSCALL( set_rt_task_param(gettid(), &params) );
}

TESTCASE(job_control_non_rt, ALL,
	 "reject job control for non-rt tasks")
{
	unsigned int job_no;

	SYSCALL_FAILS( EINVAL, sleep_next_period() );

	SYSCALL_FAILS( EINVAL, wait_for_job_release(0) );

	SYSCALL_FAILS( EPERM, get_job_no(&job_no) );
}


TESTCASE(rt_fork_non_rt, LITMUS,
	 "children of RT tasks are not automatically RT tasks")
{
	unsigned int pid, job_no;
	int status;

	SYSCALL( sporadic_partitioned(10, 100, 0) );
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
		SYSCALL( wait_for_job_release(20) );
		SYSCALL( get_job_no(&job_no) );

		SYSCALL( task_mode(BACKGROUND_TASK) );

		SYSCALL( waitpid(pid, &status, 0) );

		ASSERT(WEXITSTATUS(status) == 0);
	}
}
