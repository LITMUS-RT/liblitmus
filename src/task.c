#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>

#include "litmus.h"
#include "internal.h"

static void tperrorx(char* msg) 
{
	fprintf(stderr, 
		"Task %d: %s: %m",
		getpid(), msg);
	exit(-1);
}

/* common launch routine */
int __launch_rt_task(rt_fn_t rt_prog, void *rt_arg, rt_setup_fn_t setup, 
		     void* setup_arg) 
{
	int ret;
	int rt_task = fork();

	if (rt_task == 0) {
		/* we are the real-time task 
		 * launch task and die when it is done
		 */		
		rt_task = getpid();		
		ret = setup(rt_task, setup_arg);
		if (ret < 0)
			tperrorx("could not setup task parameters");
		ret = task_mode(LITMUS_RT_TASK);
		if (ret < 0)
			tperrorx("could not become real-time task");
		exit(rt_prog(rt_arg));
	}

	return rt_task;
}

struct create_rt_param {
	int cpu;
	int wcet;
	int period;
	task_class_t class;
};

int setup_create_rt(int pid, struct create_rt_param* arg)
{
	rt_param_t params;
	params.period      = arg->period;
	params.exec_cost   = arg->wcet;
	params.cpu         = arg->cpu;
	params.cls	   = arg->class;
	return set_rt_task_param(pid, &params);
}

int __create_rt_task(rt_fn_t rt_prog, void *arg, int cpu, int wcet, int period,
		     task_class_t class) 
{
	struct create_rt_param params;
	params.cpu = cpu;
	params.period = period;
	params.wcet = wcet;
	params.class = class;
	return __launch_rt_task(rt_prog, arg, 
				(rt_setup_fn_t) setup_create_rt, &params);
}

int create_rt_task(rt_fn_t rt_prog, void *arg, int cpu, int wcet, int period) {
	return __create_rt_task(rt_prog, arg, cpu, wcet, period, RT_CLASS_HARD);
}

