#ifndef LITMUS_H
#define LITMUS_H

#include <sys/types.h>

/* This flag is needed to start new RT tasks in STOPPED state	*/
/* Task is going to run in realtime mode */
#define CLONE_REALTIME		0x10000000	

typedef int (*rt_fn_t)(void*);
typedef int (*rt_setup_fn_t)(int pid, void* arg);

/*	Litmus scheduling policies	*/
typedef enum {
	SCHED_LINUX 		=  0,
	SCHED_PFAIR 		=  1,
	SCHED_PART_EDF 		=  3,
	SCHED_GLOBAL_EDF 	=  5,
	SCHED_PFAIR_DESYNC 	=  6,
	SCHED_GLOBAL_EDF_NP 	=  7,	
	SCHED_EDF_HSB		=  9,
	SCHED_GSN_EDF		= 10,
	SCHED_PSN_EDF		= 11,
	SCHED_ADAPTIVE		= 12,
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

typedef int sema_id;     /* ID of a semaphore in the Linux kernel */
typedef int pi_sema_id;  /* ID of a PI semaphore in the Linux kernel */
typedef int srp_sema_id; /* ID of an SRP "semaphore" in the Linux kernel */

typedef int pid_t;	 /* PID of a task */

/* X */
#define set_param(t,p,e) do{\
			(t).is_realtime=1;\
			(t).exec_cost=(e);\
			(t).period=(p);\
			}while(0);

/*	scheduler modes */
#define MODE_NON_RT 0
#define MODE_RT_RUN 1

spolicy sched_getpolicy(void);
int set_rt_mode(int mode);
int set_rt_task_param(pid_t pid, rt_param_t* param);
int get_rt_task_param(pid_t pid, rt_param_t* param);
int prepare_rt_task(pid_t pid);
int sleep_next_period(void);


enum {
	LITMUS_RESERVED_RANGE = 1024,
} SCHED_SETUP_CMD;

int scheduler_setup(int cmd, void* param);



int pi_sema_init(void);
int pi_down(pi_sema_id sem_id);
int pi_up(pi_sema_id sem_id);
int pi_sema_free(pi_sema_id sem_id);
int srp_sema_init(void);
int srp_down(srp_sema_id sem_id);
int srp_up(srp_sema_id sem_id);
int reg_task_srp_sem(srp_sema_id sem_id, pid_t t_pid);
int srp_sema_free(srp_sema_id sem_id);
int get_job_no(unsigned int* job_no);
int wait_for_job_release(unsigned int job_no);

/*  library functions */
void init_litmus(void);
/* exit is currently unused, but was needed for syscall
 * tracing and may be needed in the future. Leave it in
 * for the purpose of source code compatability.
 */
#define exit_litmus() {}



int create_rt_task(rt_fn_t rt_prog, void *arg, int cpu, int wcet, int period);
int __create_rt_task(rt_fn_t rt_prog, void *arg, int cpu, int wcet, 
		     int period, task_class_t cls);
const char* get_scheduler_name(spolicy scheduler);
void show_rt_param(rt_param_t* tp);
task_class_t str2class(const char* str);

void enter_np(void);
void exit_np(void);

int litmus_task_active();


/* low level operations, not intended for API use */
int fork_rt(void);
int __launch_rt_task(rt_fn_t rt_prog, void *rt_arg, 
		     rt_setup_fn_t setup, void* setup_arg);

#endif
