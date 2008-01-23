#include <unistd.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <signal.h>
#include <sys/mman.h>

#include "litmus.h"
#include "internal.h"

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
	case SCHED_PART_EDF:
		name = "Partioned EDF";
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
	case SCHED_ADAPTIVE:
		name = "ADAPTIVE";
		break;
	default:
		name = "Unkown";
		break;
	}
	return name;
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

int sporadic_task(unsigned long e, unsigned long p, 
		  int cpu, task_class_t cls)
{
	rt_param_t param;
	param.exec_cost = e;
	param.period    = p;
	param.cpu       = cpu;
	param.cls       = cls;
	return set_rt_task_param(getpid(), &param);
}


static int exit_requested = 0;

static void sig_handler(int sig)
{
	exit_requested = 1;
}

int litmus_task_active(void)
{
	return !exit_requested;
}


int init_kernel_iface(void);

int init_litmus(void)
{
	int ret, ret1, ret2;

	ret1 = ret = mlockall(MCL_CURRENT | MCL_FUTURE);
	check("mlockall()");
	ret2 = ret = init_kernel_iface();
	check("kernel <-> user space interface initialization");

	signal(SIGINT, sig_handler);
	signal(SIGTERM, sig_handler);
	signal(SIGHUP, sig_handler);
	signal(SIGUSR1, SIG_IGN);
	return ret1 == 0 && ret2 == 0;
}

void exit_litmus(void)
{
	/* nothing to do in current version */
}
