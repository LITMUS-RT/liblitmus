#include <unistd.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <signal.h>
#include <sys/mman.h>

#include "litmus.h"
#include "internal.h"

void show_rt_param(struct rt_task* tp) 
{
	printf("rt params:\n\t"
	       "exec_cost:\t%llu\n\tperiod:\t\t%llu\n\tcpu:\t%d\n",
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

#define NS_PER_MS 1000000

int sporadic_task(lt_t e, lt_t p, lt_t phase,
		  int cpu, task_class_t cls)
{
	struct rt_task param;
	param.exec_cost = e * NS_PER_MS;
	param.period    = p * NS_PER_MS;
	param.cpu       = cpu;
	param.cls       = cls;
	param.phase	= phase;
	return set_rt_task_param(gettid(), &param);
}

int init_kernel_iface(void);

int init_litmus(void)
{
	int ret, ret2;

	ret = mlockall(MCL_CURRENT | MCL_FUTURE);
	check("mlockall()");
	ret2 = init_rt_thread();
	return (ret == 0) && (ret2 == 0) ? 0 : -1;
}

int init_rt_thread(void)
{
	int ret;

        ret = init_kernel_iface();
	check("kernel <-> user space interface initialization");
	return ret;
}

void exit_litmus(void)
{
	/* nothing to do in current version */
}
