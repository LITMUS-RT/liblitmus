/* To get syscall() we need to define _GNU_SOURCE 
 * in modern glibc versions.
 */
#define _GNU_SOURCE
#include <unistd.h>

#include "litmus.h"

#include "syscalls.h"

struct np_flag;

/*	Litmus syscalls definitions */
#define __NR_sched_setpolicy 	320
#define __NR_sched_getpolicy 	321
#define __NR_set_rt_mode	322
#define __NR_set_rt_task_param	323
#define __NR_get_rt_task_param	324
#define __NR_sleep_next_period  326
#define __NR_scheduler_setup	327
#define __NR_register_np_flag   328
#define __NR_signal_exit_np     329
#define __NR_od_openx		330
#define __NR_od_close		331
#define __NR_pi_down		332
#define __NR_pi_up		333
#define __NR_srp_down		334
#define __NR_srp_up		335
#define __NR_reg_task_srp_sem	336
#define __NR_get_job_no		337
#define __NR_wait_for_job_release 	338
#define __NR_set_service_levels 	339
#define __NR_get_cur_service_level 	340
#define __NR_reg_ics_cb			341
#define __NR_start_wcs			342
#define __NR_task_mode		 	343

/*	Syscall stub for setting RT mode and scheduling options */
_syscall0(spolicy, sched_getpolicy);
_syscall1(int,     set_rt_mode,       int,         arg1);
_syscall2(int,     set_rt_task_param, pid_t,       pid,    rt_param_t*, arg1);
_syscall2(int,     get_rt_task_param, pid_t,       pid,    rt_param_t*, arg1);
_syscall0(int,     sleep_next_period);
_syscall2(int,     scheduler_setup,   int,         cmd,    void*,       param);
_syscall1(int,     register_np_flag, struct np_flag*, flag);
_syscall0(int,     signal_exit_np);

_syscall4(int,     od_openx,           int,  fd, obj_type_t, type, int, obj_id,
	  void*,   config);
_syscall1(int,     od_close,          int,  od);
_syscall1(int,     pi_down,           int,  od);
_syscall1(int,     pi_up,             int,  od);
_syscall1(int,     srp_down,          int,  od);
_syscall1(int,     srp_up,            int,  od);
_syscall1(int,     reg_task_srp_sem,  int,  od);

_syscall1(int,     get_job_no,      unsigned int*, job_no);
_syscall1(int,     wait_for_job_release, unsigned int, job_no);

_syscall1(int,     start_wcs,         int,  od);
_syscall1(int,     reg_ics_cb,        struct ics_cb*, ics_cb);
_syscall1(int,     task_mode, int, target_mode);
