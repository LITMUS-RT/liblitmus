#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <sys/types.h>
#include <unistd.h>
#include <errno.h>
#include <signal.h>

#include "litmus.h"

/* this is missing in newer linux/unistd.h versions */

#define _syscall0(type,name) \
type name(void) \
{\
        return syscall(__NR_##name);\
}

#define _syscall1(type,name,type1,arg1) \
type name(type1 arg1) \
{\
        return syscall(__NR_##name, arg1);\
}


#define _syscall2(type,name,type1,arg1,type2,arg2) \
type name(type1 arg1,type2 arg2) \
{\
        return syscall(__NR_##name, arg1, arg2);\
}


/* clear the TID in the child */
#define CLONE_CHILD_CLEARTID	0x00200000	
/* set the TID in the child */
#define CLONE_CHILD_SETTID	0x01000000
/* don't let the child run before we have completed setup */
#define CLONE_STOPPED		0x02000000	
/* litmus clone flag */
#define CLONE_REALTIME		0x10000000

/* CLONE_REALTIME is necessary because CLONE_STOPPED will put a SIGSTOP in 
 * the pending signal queue. Thus the first thing a newly created task will
 * do after it is released is to stop, which is not what we want
 * 
 * CLONE_REALTIME just sets the status to TASK_STOPPED without queueing a 
 * signal.
 */


/* this is essentially a fork with CLONE_STOPPED */
/* #define CLONE_LITMUS  CLONE_STOPPED | CLONE_CHILD_CLEARTID | CLONE_CHILD_SETTID */
#define CLONE_LITMUS CLONE_REALTIME | CLONE_CHILD_CLEARTID | CLONE_CHILD_SETTID

/* we need to override libc because it wants to be clever 
 * and rejects our call without presenting it even to the kernel
 */
#define __NR_raw_clone		120


_syscall2(int, raw_clone, unsigned long, flags, unsigned long, child_stack)


const char* get_scheduler_name(spolicy scheduler) 
{
	const char* name;

	switch (scheduler){
	case SCHED_LINUX :
		name = "Linux";
		break;
	case SCHED_PFAIR:
		name = "Pfair";
		break;
	case SCHED_PFAIR_STAGGER:
		name = "Pfair (staggered)";
		break;
	case SCHED_PART_EDF:
		name = "Partioned EDF";
		break;
	case SCHED_PART_EEVDF:
		name = "Partioned EEVDF";
		break;
	case SCHED_GLOBAL_EDF:
		name = "Global EDF";
		break;
	case SCHED_EDF_HSB:
		name = "EDF-HSB";
		break;
	case SCHED_GSN_EDF:
		name = "GSN-EDF";
		break;
	case SCHED_PSN_EDF:
		name = "PSN-EDF";
		break;
	default:
		name = "Unkown";
		break;
	}
	return name;
}

int create_rt_task(rt_fn_t rt_prog, void *arg, int cpu, int wcet, int period) {
	return __create_rt_task(rt_prog, arg, cpu, wcet, period, RT_CLASS_HARD);
}

int __create_rt_task(rt_fn_t rt_prog, void *arg, int cpu, int wcet, int period,
		     task_class_t class) 
{
	int ret;
	rt_param_t params;
	int rt_task = raw_clone(CLONE_LITMUS, 0);

	if (rt_task < 0) 
		return rt_task;

	if (rt_task > 0) {
		/* we are the controller task */			
		params.period      = period;
		params.exec_cost   = wcet;
		params.cpu         = cpu;
		params.cls	   = class;
		ret = set_rt_task_param(rt_task, &params);
		if (ret < 0) {
			/* we have a problem: we created the task but
			 * for some stupid reason we cannot set the real-time
			 * parameters. We must clean up the stopped task.
			 */
			kill(rt_task, SIGKILL);
			/* syscall will have set errno, we don't have to do
			 * anything
			 */
			return -1;
		}
		ret = prepare_rt_task(rt_task);
		if (ret < 0) {
			/* same problem as above*/
			//kill(rt_task, SIGKILL);
			rt_task = -1;
		}
		return rt_task;
	}
	else {
		/* we are the real-time task 
		 * launch task and die when it is done
		 */		
		
		exit(rt_prog(arg));
	}
}


void show_rt_param(rt_param_t* tp) 
{
	printf("rt params:\n\t"
	       "exec_cost:\t%ld\n\tperiod:\t\t%ld\n\tcpu:\t%d\n",
	       tp->exec_cost, tp->period, tp->cpu);
}

task_class_t str2class(const char* str) 
{
	if      (!strcmp(str, "hrt"))
		return RT_CLASS_HARD;
	else if (!strcmp(str, "srt"))
		return RT_CLASS_SOFT;
	else if (!strcmp(str, "be"))
		return RT_CLASS_BEST_EFFORT;
	else
		return -1;
}


/*	Litmus syscalls definitions */
#define __NR_sched_setpolicy 	  320
#define __NR_sched_getpolicy 	  321
#define __NR_set_rt_mode	  322
#define __NR_set_rt_task_param	  323
#define __NR_get_rt_task_param	  324
#define __NR_prepare_rt_task	  325
#define __NR_reset_stat		  326
#define __NR_sleep_next_period    327
#define __NR_scheduler_setup	  328
#define __NR_enter_np		  329
#define __NR_exit_np		  330
#define __NR_pi_sema_init         331
#define __NR_pi_down              332
#define __NR_pi_up                333
#define __NR_pi_sema_free         334
#define __NR_sema_init            335
#define __NR_down                 336
#define __NR_up                   337
#define __NR_sema_free            338
#define __NR_srp_sema_init        339
#define __NR_srp_down             340
#define __NR_srp_up               341
#define __NR_reg_task_srp_sem     342
#define __NR_srp_sema_free        343
#define __NR_query_job_no         344
#define __NR_wait_for_job_release 345


/*	Syscall stub for setting RT mode and scheduling options */
_syscall1(spolicy, sched_setpolicy,   spolicy, arg1);
_syscall0(spolicy, sched_getpolicy);
_syscall1(int,     set_rt_mode,       int,         arg1);
_syscall2(int,     set_rt_task_param, pid_t,       pid,    rt_param_t*, arg1);
_syscall2(int,     get_rt_task_param, pid_t,       pid,    rt_param_t*, arg1);
_syscall1(int, 	   prepare_rt_task,   pid_t,       pid);
_syscall0(int,     reset_stat);
_syscall0(int,     sleep_next_period);
_syscall2(int,     scheduler_setup,   int,         cmd,    void*,       param);
_syscall0(int,     enter_np);
_syscall0(int,     exit_np);
_syscall0(int,	   pi_sema_init);
_syscall1(int,     pi_down,           pi_sema_id,  sem_id);
_syscall1(int,     pi_up,             pi_sema_id,  sem_id);
_syscall1(int,     pi_sema_free,      pi_sema_id,  sem_id);
_syscall0(int,     sema_init);
_syscall1(int,     down,              sema_id,     sem_id);
_syscall1(int,     up,                sema_id,     sem_id);
_syscall1(int,     sema_free,         sema_id,     sem_id);
_syscall0(int,     srp_sema_init);
_syscall1(int,     srp_down,          srp_sema_id, sem_id);
_syscall1(int,     srp_up,            srp_sema_id, sem_id);
_syscall2(int,     reg_task_srp_sem,  srp_sema_id, sem_id, pid_t,       t_pid);
_syscall1(int,     srp_sema_free,     srp_sema_id, sem_id);
_syscall1(int,     query_job_no,      unsigned int*, job_no);
_syscall1(int,     wait_for_job_release, unsigned int, job_no);

