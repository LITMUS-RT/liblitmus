/* To get syscall() we need to define _GNU_SOURCE
 * in modern glibc versions.
 */

/* imported from the kernel source tree */
#include "asm/unistd.h"

/* for syscall() */
#include <unistd.h>

#include "litmus.h"
#include "internal.h"

/*	Syscall stub for setting RT mode and scheduling options */

pid_t gettid(void)
{
	return syscall(__NR_gettid);
}

int set_rt_task_param(pid_t pid, struct rt_task *param)
{
	union litmus_syscall_args args;
	args.get_set_task_param.pid = pid;
	args.get_set_task_param.param = param;
	return litmus_syscall(LRT_set_rt_task_param, (unsigned long) &args);
}

int get_rt_task_param(pid_t pid, struct rt_task *param)
{
	union litmus_syscall_args args;
	args.get_set_task_param.pid = pid;
	args.get_set_task_param.param = param;
	return litmus_syscall(LRT_get_rt_task_param, (unsigned long) &args);
}

int reservation_create(int rtype, void *config)
{
	union litmus_syscall_args args;
	args.reservation_create.type  = rtype;
	args.reservation_create.config = config;
	return litmus_syscall(LRT_reservation_create, (unsigned long) &args);
}

int sleep_next_period(void)
{
	return litmus_syscall(LRT_complete_job, 0);
}

int od_openx(int fd, obj_type_t type, int obj_id, void *config)
{
	union litmus_syscall_args args;
	args.od_open.fd = fd;
	args.od_open.obj_type = type;
	args.od_open.obj_id = obj_id;
	args.od_open.config = config;
	return litmus_syscall(LRT_od_open, (unsigned long) &args);
}

int od_close(int od)
{
	return litmus_syscall(LRT_od_close, od);
}

int litmus_lock(int od)
{
	return litmus_syscall(LRT_litmus_lock, od);
}

int litmus_unlock(int od)
{
	return litmus_syscall(LRT_litmus_unlock, od);
}

int get_job_no(unsigned int *job_no)
{
	struct control_page* cp = get_ctrl_page();
	if (likely(cp != NULL)) {
		*job_no = cp->job_index;
		return 0;
	} else {
		return -1;
	}
}

int wait_for_job_release(unsigned int job_no)
{
	return litmus_syscall(LRT_wait_for_job_release, job_no);
}

int wait_for_ts_release(void)
{
	return litmus_syscall(LRT_wait_for_ts_release, 0);
}

int release_ts(lt_t *when)
{
	return litmus_syscall(LRT_release_ts, (unsigned long) when);
}

int null_call(cycles_t *timestamp)
{
	return litmus_syscall(LRT_null_call, (unsigned long) timestamp);
}

int get_current_budget(
	lt_t *expended,
	lt_t *remaining)
{
	union litmus_syscall_args args;
	args.get_current_budget.expended = expended;
	args.get_current_budget.remaining = remaining;
	return litmus_syscall(LRT_get_current_budget, (unsigned long) &args);
}
