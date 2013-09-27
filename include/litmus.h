#ifndef LITMUS_H
#define LITMUS_H

#ifdef __cplusplus
extern "C" {
#endif

#include <sys/types.h>
#include <stdint.h>

/* Include kernel header.
 * This is required for the rt_param
 * and control_page structures.
 */
#include "litmus/rt_param.h"

#include "asm/cycles.h" /* for null_call() */

#include "migration.h"

#define SCHED_LITMUS 6

void init_rt_task_param(struct rt_task* param);
int set_rt_task_param(pid_t pid, struct rt_task* param);
int get_rt_task_param(pid_t pid, struct rt_task* param);

/* Release-master-aware functions for getting the first
 * CPU in a particular cluster or partition. Use these
 * to set rt_task::cpu for cluster/partitioned scheduling.
 */
int partition_to_cpu(int partition);
int cluster_to_first_cpu(int cluster, int cluster_size);

/* Convenience functions for setting up real-time tasks.
 * Default behaviors set by init_rt_task_params() used.
 * Also sets affinity masks for clustered/partitions
 * functions. Time units in nanoseconds. */
int sporadic_global(lt_t e_ns, lt_t p_ns);
int sporadic_partitioned(lt_t e_ns, lt_t p_ns, int partition);
int sporadic_clustered(lt_t e_ns, lt_t p_ns, int cluster, int cluster_size);

/* simple time unit conversion macros */
#define s2ns(s)   ((s)*1000000000LL)
#define s2us(s)   ((s)*1000000LL)
#define s2ms(s)   ((s)*1000LL)
#define ms2ns(ms) ((ms)*1000000LL)
#define ms2us(ms) ((ms)*1000LL)
#define us2ns(us) ((us)*1000LL)

/* file descriptor attached shared objects support */
typedef enum  {
	FMLP_SEM	= 0,
	SRP_SEM		= 1,
	MPCP_SEM	= 2,
	MPCP_VS_SEM	= 3,
	DPCP_SEM	= 4,
	PCP_SEM         = 5,
} obj_type_t;

int lock_protocol_for_name(const char* name);
const char* name_for_lock_protocol(int id);

int od_openx(int fd, obj_type_t type, int obj_id, void* config);
int od_close(int od);

static inline int od_open(int fd, obj_type_t type, int obj_id)
{
	return od_openx(fd, type, obj_id, 0);
}

int litmus_open_lock(
	obj_type_t protocol,	/* which locking protocol to use, e.g., FMLP_SEM */
	int lock_id,		/* numerical id of the lock, user-specified */
	const char* name_space,	/* path to a shared file */
	void *config_param);	/* any extra info needed by the protocol (such
				 * as CPU under SRP and PCP), may be NULL */

/* real-time locking protocol support */
int litmus_lock(int od);
int litmus_unlock(int od);

/* job control*/
int get_job_no(unsigned int* job_no);
int wait_for_job_release(unsigned int job_no);
int sleep_next_period(void);

/*  library functions */
int  init_litmus(void);
int  init_rt_thread(void);
void exit_litmus(void);

/* A real-time program. */
typedef int (*rt_fn_t)(void*);

/* These two functions configure the RT task to use enforced exe budgets.
 * Partitioned scheduling: cluster = desired partition, cluster_size = 1
 * Global scheduling: cluster = 0, cluster_size = 0
 */
int create_rt_task(rt_fn_t rt_prog, void *arg, int cluster, int cluster_size,
			lt_t wcet, lt_t period, unsigned int prio);
int __create_rt_task(rt_fn_t rt_prog, void *arg, int cluster, int cluster_size,
			lt_t wcet, lt_t period, unsigned int prio, task_class_t cls);

/*	per-task modes */
enum rt_task_mode_t {
	BACKGROUND_TASK = 0,
	LITMUS_RT_TASK  = 1
};
int task_mode(int target_mode);

void show_rt_param(struct rt_task* tp);
task_class_t str2class(const char* str);

/* non-preemptive section support */
void enter_np(void);
void exit_np(void);
int  requested_to_preempt(void);

/* task system support */
int wait_for_ts_release(void);
int release_ts(lt_t *delay);
int get_nr_ts_release_waiters(void);
int read_litmus_stats(int *ready, int *total);

/* sleep for some number of nanoseconds */
int lt_sleep(lt_t timeout);

/* CPU time consumed so far in seconds */
double cputime(void);

/* wall-clock time in seconds */
double wctime(void);

/* semaphore allocation */

static inline int open_fmlp_sem(int fd, int name)
{
	return od_open(fd, FMLP_SEM, name);
}

static inline int open_srp_sem(int fd, int name)
{
	return od_open(fd, SRP_SEM, name);
}

static inline int open_pcp_sem(int fd, int name, int cpu)
{
	return od_openx(fd, PCP_SEM, name, &cpu);
}

static inline int open_mpcp_sem(int fd, int name)
{
	return od_open(fd, MPCP_SEM, name);
}

static inline int open_dpcp_sem(int fd, int name, int cpu)
{
	return od_openx(fd, DPCP_SEM, name, &cpu);
}


/* syscall overhead measuring */
int null_call(cycles_t *timestamp);

/*
 * get control page:
 * atm it is used only by preemption migration overhead code
 * but it is very general and can be used for different purposes
 */
struct control_page* get_ctrl_page(void);

#ifdef __cplusplus
}
#endif
#endif
