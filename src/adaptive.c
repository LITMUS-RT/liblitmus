#include <sys/types.h>
#include <unistd.h>

#include "litmus.h"
#include "internal.h"
#include "syscalls.h"

#include "adaptive.h"

#define __NR_set_service_levels 	346
#define __NR_get_cur_service_level 	347



int set_service_levels(pid_t pid, 
		       unsigned int nr_levels,
		       service_level_t* levels)
{
	return syscall(__NR_set_service_levels, pid, nr_levels, levels);
}


int get_cur_service_level(void)
{
	return syscall(__NR_get_cur_service_level);
}


struct adaptive_param {
	unsigned int 		no_levels;
	service_level_t* 	levels;
};

int setup_adaptive(int pid, struct adaptive_param* arg)
{
	return set_service_levels(pid, arg->no_levels, arg->levels);
}

int create_adaptive_rt_task(rt_fn_t rt_prog, void *arg, 
			    unsigned int no_levels, service_level_t* levels) 
{
	struct adaptive_param p;
	p.no_levels = no_levels;
	p.levels    = levels;
	return __launch_rt_task(rt_prog, arg,
				(rt_setup_fn_t) setup_adaptive, &p);
}

