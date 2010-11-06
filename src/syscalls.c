/* To get syscall() we need to define _GNU_SOURCE
 * in modern glibc versions.
 */

/* imported from the kernel source tree */
#include "asm/unistd.h"

/* for syscall() */
#include <unistd.h>

//#include <sys/types.h>

#include "litmus.h"

/*	Syscall stub for setting RT mode and scheduling options */

pid_t gettid(void)
{
	return syscall(__NR_gettid);
}

int set_rt_task_param(pid_t pid, struct rt_task *param)
{
	return syscall(__NR_set_rt_task_param, pid, param);
}

int get_rt_task_param(pid_t pid, struct rt_task *param)
{
	return syscall(__NR_get_rt_task_param, pid, param);
}

int sleep_next_period(void)
{
	return syscall(__NR_complete_job);
}

int od_openx(int fd, obj_type_t type, int obj_id, void *config)
{
	return syscall(__NR_od_open, fd, type, obj_id, config);
}

int od_close(int od)
{
	return syscall(__NR_od_close, od);
}

int fmlp_down(int od)
{
	return syscall(__NR_fmlp_down, od);
}

int fmlp_up(int od)
{
	return syscall(__NR_fmlp_up, od);
}

int srp_down(int od)
{
	return syscall(__NR_srp_down, od);
}

int srp_up(int od)
{
	return syscall(__NR_srp_up, od);
}

int get_job_no(unsigned int *job_no)
{
	return syscall(__NR_query_job_no, job_no);
}

int wait_for_job_release(unsigned int job_no)
{
	return syscall(__NR_wait_for_job_release, job_no);
}

int sched_setscheduler(pid_t pid, int policy, int* priority)
{
	return syscall(__NR_sched_setscheduler, pid, policy, priority);
}

int sched_getscheduler(pid_t pid)
{
	return syscall(__NR_sched_getscheduler, pid);
}

int wait_for_ts_release(void)
{
	return syscall(__NR_wait_for_ts_release);
}

int release_ts(lt_t *delay)
{
	return syscall(__NR_release_ts, delay);
}

int null_call(cycles_t *timestamp)
{
	return syscall(__NR_null_call, timestamp);
}
