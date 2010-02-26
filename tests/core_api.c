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
