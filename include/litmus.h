#ifndef LITMUS_H
#define LITMUS_H

#include <sys/types.h>

/* A real-time program. */
typedef int (*rt_fn_t)(void*);

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

typedef int pid_t;	 /* PID of a task */

/*	scheduler modes */
#define MODE_NON_RT 0
#define MODE_RT_RUN 1

spolicy sched_getpolicy(void);
int set_rt_mode(int mode);
int set_rt_task_param(pid_t pid, rt_param_t* param);
int get_rt_task_param(pid_t pid, rt_param_t* param);

/* setup helper */
int sporadic_task(unsigned long exec_cost, unsigned long period, 
		  int partition, task_class_t cls);

#define sporadic_global(e, p) \
	sporadic_task(e, p, 0, RT_CLASS_SOFT)
#define sporadic_partitioned(e, p, cpu) \
	sporadic_task(e, p, cpu, RT_CLASS_SOFT)


/* deprecated */
enum {
	LITMUS_RESERVED_RANGE = 1024,
} SCHED_SETUP_CMD;
int scheduler_setup(int cmd, void* param);

/* file descriptor attached shared objects support */
typedef enum  {
	PI_SEM 		= 0,
	SRP_SEM		= 1,
	ICS_ID		= 2,
} obj_type_t;

int od_openx(int fd, obj_type_t type, int obj_id, void* config);
int od_close(int od);

static inline int od_open(int fd, obj_type_t type, int obj_id)
{
	return od_openx(fd, type, obj_id, 0);
}

/* FMLP support */
int pi_down(int od);
int pi_up(int od);
int srp_down(int od);
int srp_up(int od);
int reg_task_srp_sem(int od);

/* job control*/
int get_job_no(unsigned int* job_no);
int wait_for_job_release(unsigned int job_no);
int sleep_next_period(void);

/* interruptible critical section support */
#define MAX_ICS_NESTING 	16
#define ICS_STACK_EMPTY 	(-1)

struct ics_descriptor {
	/* ICS id, only read by kernel */
	int 	id;
	/* rollback program counter, only read by kernel */
	void* 	pc; 
	/* rollback stack pointer, not used by kernel */
	void*	 sp;
	/* retry flag, not used by kernel */
	int* 	retry;
};

/* ICS control block */
struct ics_cb {
	/* Points to the top-most valid entry.
	 * -1 indicates an empty stack.
	 * Read and written by kernel.
	 */
	int			top;	
	struct ics_descriptor	ics_stack[MAX_ICS_NESTING];
};

int reg_ics_cb(struct ics_cb* ics_cb);
int start_wcs(int od);

/*  library functions */
int  init_litmus(void);
int  init_rt_thread(void);
void exit_litmus(void);

int create_rt_task(rt_fn_t rt_prog, void *arg, int cpu, int wcet, int period);
int __create_rt_task(rt_fn_t rt_prog, void *arg, int cpu, int wcet, 
		     int period, task_class_t cls);

/*	per-task modes */
enum rt_task_mode_t {
	BACKGROUND_TASK = 0,
	LITMUS_RT_TASK  = 1
};
int task_mode(int target_mode);

const char* get_scheduler_name(spolicy scheduler);
void show_rt_param(rt_param_t* tp);
task_class_t str2class(const char* str);

/* non-preemptive section support */
void enter_np(void);
void exit_np(void);

#endif
