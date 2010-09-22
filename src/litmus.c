#include <unistd.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <signal.h>
#include <sys/mman.h>

#include <sched.h> /* for cpu sets */

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

/* only for best-effort execution: migrate to target_cpu */
int be_migrate_to(int target_cpu)
{
	cpu_set_t cpu_set;

	CPU_ZERO(&cpu_set);
	CPU_SET(target_cpu, &cpu_set);
	return sched_setaffinity(0, sizeof(cpu_set_t), &cpu_set);
}

int sporadic_task(lt_t e, lt_t p, lt_t phase,
		  int cpu, task_class_t cls,
		  budget_policy_t budget_policy, int set_cpu_set)
{
	return sporadic_task_ns(e * NS_PER_MS, p * NS_PER_MS, phase * NS_PER_MS,
				cpu, cls, budget_policy, set_cpu_set);
}

int sporadic_task_ns(lt_t e, lt_t p, lt_t phase,
			int cpu, task_class_t cls,
			budget_policy_t budget_policy, int set_cpu_set)
{
	struct rt_task param;
	int ret;

	/* Zero out first --- this is helpful when we add plugin-specific
	 * parameters during development.
	 */
	memset(&param, 0, sizeof(param));

	param.exec_cost = e;
	param.period    = p;
	param.cpu       = cpu;
	param.cls       = cls;
	param.phase	= phase;
	param.budget_policy = budget_policy;

	if (set_cpu_set) {
		ret = be_migrate_to(cpu);
		check("migrate to cpu");
	}
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
