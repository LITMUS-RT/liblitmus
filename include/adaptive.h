#ifndef ADAPTIVE_H
#define ADAPTIVE_H

#define FP_SHIFT 10
typedef struct
{
	long val;
} fp_t;

static inline fp_t f2fp(double f)
{
	return (fp_t) {f * (1 << FP_SHIFT)};
}

#define MAX_SERVICE_LEVELS 10
typedef struct {
	fp_t	 	weight;
	unsigned long 	period;
	fp_t		value;
} service_level_t;

int set_service_levels(pid_t pid, 
		       unsigned int nr_levels,
		       service_level_t* levels);

int get_cur_service_level(void);

int create_adaptive_rt_task(rt_fn_t rt_prog, void *arg, 
			    unsigned int no_levels, service_level_t* levels);


#endif
