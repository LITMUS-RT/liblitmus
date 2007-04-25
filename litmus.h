#ifndef LITMUS_H
#define LITMUS_H

#include <sys/types.h>

/* This flag is needed to start new RT tasks in STOPPED state	*/
/* Task is going to run in realtime mode */
#define CLONE_REALTIME		0x10000000	

typedef int (*rt_fn_t)(void*);

/*	Litmus scheduling policies	*/
typedef enum {
	SCHED_LINUX 		=  0,
	SCHED_PFAIR 		=  1,
	SCHED_PFAIR_STAGGER 	=  2,
	SCHED_PART_EDF 		=  3,
	SCHED_PART_EEVDF 	=  4,
	SCHED_GLOBAL_EDF 	=  5,
	SCHED_PFAIR_DESYNC 	=  6,
	SCHED_GLOBAL_EDF_NP 	=  7,	
	SCHED_CUSTOM 		=  8,
	SCHED_EDF_HSB		=  9,
	SCHED_GSN_EDF		= 10
} spolicy;

/* different types of clients */
typedef enum {
	RT_CLASS_HARD,	
	RT_CLASS_SOFT,
	RT_CLASS_BEST_EFFORT
} task_class_t;

/*	Task RT params for schedulers */
/*	RT task parameters for scheduling extensions 	
*	These parameters are inherited during clone and therefore must
*	be explicitly set up before the task set is launched.
*/
typedef struct rt_param {	
	/* 	Execution cost 	*/
	unsigned long exec_cost;	
	/*	Period		*/
	unsigned long period;			
	/*	Partition 	*/
	unsigned int cpu;
	/* 	type of task	*/
	task_class_t  	cls;
} rt_param_t;

typedef int sema_id; /* ID of a semaphore in the Linux kernel */
typedef int pi_sema_id; /* ID of a PI semaphore in the Linux kernel */

#define set_param(t,p,e) do{\
			(t).is_realtime=1;\
			(t).exec_cost=(e);\
			(t).period=(p);\
			}while(0);

/*	scheduler modes */
#define MODE_NON_RT 0
#define MODE_RT_RUN 1

spolicy sched_setpolicy(spolicy policy);
spolicy sched_getpolicy(void);
int set_rt_mode(int mode);
int set_rt_task_param(pid_t pid, rt_param_t* param);
int get_rt_task_param(pid_t pid, rt_param_t* param);
int prepare_rt_task(pid_t pid);
int tear_down_task(pid_t pid, int sig);
int reset_stat(void);
int sleep_next_period(void);
int scheduler_setup(int cmd, void* param);
int enter_np(void);
int exit_np(void);
int pi_sema_init(void);
int pi_down(pi_sema_id sem_id);
int pi_up(pi_sema_id sem_id);
int pi_sema_free(pi_sema_id sem_id);
int sema_init(void);
int down(sema_id sem_id);
int up(sema_id sem_id);
int sema_free(sema_id sem_id);

/*  library functions */
int create_rt_task(rt_fn_t rt_prog, void *arg, int cpu, int wcet, int period);
int __create_rt_task(rt_fn_t rt_prog, void *arg, int cpu, int wcet, 
		     int period, task_class_t cls);
const char* get_scheduler_name(spolicy scheduler);
void show_rt_param(rt_param_t* tp);
task_class_t str2class(const char* str);


#endif
