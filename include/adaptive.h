#ifndef ADAPTIVE_H
#define ADAPTIVE_H

#define MAX_SERVICE_LEVELS 10

typedef struct {
	unsigned long 	exec_cost;	
	unsigned long 	period;
	/* fixed point */
	unsigned long	utility;
} service_level_t;

int set_service_levels(pid_t pid, 
		       unsigned int nr_levels,
		       service_level_t* levels);

int get_cur_service_level(void);

int create_adaptive_rt_task(rt_fn_t rt_prog, void *arg, 
			    unsigned int no_levels, service_level_t* levels);


#endif
