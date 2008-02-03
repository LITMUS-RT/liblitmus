/* To get syscall() we need to define _GNU_SOURCE 
 * in modern glibc versions.
 */
#define _GNU_SOURCE
#include <unistd.h>
#include <linux/unistd.h>
#include <sys/types.h>

#include "litmus.h"

struct np_flag;

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
	return syscall(__NR_sleep_next_period);
}

int register_np_flag(struct np_flag *flag)
{
	return syscall(__NR_register_np_flag, flag);
}

int signal_exit_np(void)
{
	return syscall(__NR_exit_np);
}

int od_openx(int fd, obj_type_t type, int obj_id, void *config)
{
	return syscall(__NR_od_open, fd, type, obj_id, config);
}

int od_close(int od)
{
	return syscall(__NR_od_close, od);
}

int pi_down(int od)
{
	return syscall(__NR_pi_down, od);
}

int pi_up(int od)
{
	return syscall(__NR_pi_up, od);
}

int srp_down(int od)
{
	return syscall(__NR_srp_down, od);
}

int srp_up(int od)
{
	return syscall(__NR_srp_up, od);
}

int reg_task_srp_sem(int od)
{
	return syscall(__NR_reg_task_srp_sem, od);
}

int get_job_no(unsigned int *job_no)
{
	return syscall(__NR_query_job_no, job_no);
}

int wait_for_job_release(unsigned int job_no)
{
	return syscall(__NR_wait_for_job_release, job_no);
}

int task_mode(int target_mode)
{
	return syscall(__NR_task_mode, target_mode);
}

