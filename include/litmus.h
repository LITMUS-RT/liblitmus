/**
 * @file litmus.h
 * Public API for LITMUS^RT
 */

/**
 * @mainpage
 * The LITMUS^RT patch is a (soft) real-time extension of the Linux kernel with a
 * focus on multiprocessor real-time scheduling and synchronization. The Linux
 * kernel is modified to support the sporadic task model and modular scheduler
 * plugins. Clustered, partitioned, and global scheduling are included, and
 * semi-partitioned scheduling is supported as well.
 *
 * \b liblitmus is the userspace API for LITMUS^RT. It consists of functions to
 * control scheduling protocols and parameters, mutexes as well as functionality
 * to create test suites.
 *
 * Example test programs can be found in \b bin/base_task.c and
 * \b bin/base_task_mt.c . Several test suites are given in the \b tests
 * directory.
 */

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
#define __user
#include "litmus/ctrlpage.h"
#undef __user

#include "asm/cycles.h" /* for null_call() */

#include "migration.h"

/**
 * @private
 * The numeric ID of the LITMUS^RT scheduling class.
 */
#define SCHED_LITMUS 7

/**
 * Initialise a real-time task param struct
 * @param param Pointer to the struct to initialise
 */
void init_rt_task_param(struct rt_task* param);

/**
 * Set real-time task parameters for given process
 * @param pid PID of process
 * @param param Real-time task parameter struct
 * @return 0 on success
 */
int set_rt_task_param(pid_t pid, struct rt_task* param);

/**
 * Get real-time task parameters for given process
 * @param pid PID of process
 * @param param Real-time task parameter struct to fill
 * @return 0 on success
 */
int get_rt_task_param(pid_t pid, struct rt_task* param);

/**
 * Create a new reservation/container (not supported by all plugins).
 * @param rtype The type of reservation to create.
 * @param config optional reservation-specific configuration (may be NULL)
 * @return 0 on success
 */
int reservation_create(int rtype, void *config);

/**
 * Convert a partition number to a CPU identifier
 * @param partition Partition number
 * @return CPU identifier for given partition
 *
 *  Release-master-aware functions for getting the first
 * CPU in a particular cluster or partition. Use these
 * to set rt_task::cpu for cluster/partitioned scheduling.
 *
 * \deprecated{Use domain_to_first_cpu() in new code.}
 */
int partition_to_cpu(int partition);

/**
 * For given cluster, return the identifier for the first associated CPU
 * @param cluster Identifier of the cluster
 * @param cluster_size Size for this cluster
 * @return Identifier for the first associated CPU
 *
 * \deprecated{Use domain_to_first_cpu() in new code.}
 */
int cluster_to_first_cpu(int cluster, int cluster_size);


/* The following three functions are convenience functions for setting up
 * real-time tasks.  Default behaviors set by init_rt_task_params() are used.
 * Also sets affinity masks for clustered/partitions functions. Time units in
 * nanoseconds. */
/**
 * Set up a sporadic task with global scheduling
 * @param e_ns Execution time in nanoseconds
 * @param p_ns Period in nanoseconds
 * @return 0 on success
 */
int sporadic_global(lt_t e_ns, lt_t p_ns);

/**
 * Set up a sporadic task with partitioned scheduling
 * @param e_ns Execution time in nanoseconds
 * @param p_ns Period in nanoseconds
 * @param partition Identifier for partition to add this task to
 * @return 0 on success
 */
int sporadic_partitioned(lt_t e_ns, lt_t p_ns, int partition);

/**
 * Set up a sporadic task with clustered scheduling
 * @param e_ns Execution time in nanoseconds
 * @param p_ns Period in nanoseconds
 * @param cluster Cluster to add this task to
 * @return 0 on success
 */
int sporadic_clustered(lt_t e_ns, lt_t p_ns, int cluster);

/* simple time unit conversion macros */
/** Convert seconds to nanoseconds
 * @param s Time units in seconds */
#define s2ns(s)   ((s)*1000000000LL)

/** Convert seconds to microseconds
  * @param s Time units in seconds */
#define s2us(s)   ((s)*1000000LL)

/** Convert seconds to milliseconds
  * @param s Time units in seconds */
#define s2ms(s)   ((s)*1000LL)

/** Convert milliseconds to nanoseconds
  * @param ms Time units in milliseconds */
#define ms2ns(ms) ((ms)*1000000LL)

/** Convert milliseconds to microseconds
  * @param ms Time units in milliseconds */
#define ms2us(ms) ((ms)*1000LL)

/** Convert microseconds to nanoseconds
  * @param us Time units in microseconds */
#define us2ns(us) ((us)*1000LL)

/** Convert nanoseconds to seconds (truncating)
 * @param ns Time units in nanoseconds */
#define ns2s(ns)   ((ns)/1000000000LL)

/**
 * Locking protocols for allocated shared objects
 */
typedef enum  {
	FMLP_SEM	= 0, /**< Fifo-based Multiprocessor Locking Protocol */
	SRP_SEM		= 1, /**< Stack Resource Protocol */
	MPCP_SEM	= 2, /**< Multiprocessor Priority Ceiling Protocol */
	MPCP_VS_SEM	= 3, /**< Multiprocessor Priority Ceiling Protocol with
						  Virtual Spinning */
	DPCP_SEM	= 4, /**< Distributed Priority Ceiling Protocol */
	PCP_SEM		= 5, /**< Priority Ceiling Protocol */
	DFLP_SEM	= 6, /**< Distributed FIFO Locking Protocol */
} obj_type_t;

/**
 * For given protocol name, return semaphore object type id
 * @param name String representation of protocol name
 * @return Object type ID as integer
 */
int lock_protocol_for_name(const char* name);
/**
 * For given semaphore object type id, return the name of the protocol
 * @param id Semaphore object type ID
 * @return Name of the locking protocol
 */
const char* name_for_lock_protocol(int id);

/**
 * @private
 * Do a syscall for opening a generic lock
 */
int od_openx(int fd, obj_type_t type, int obj_id, void* config);
/**
 * Close a lock, given its object descriptor
 * @param od Object descriptor for lock to close
 * @return 0 Iff the lock was successfully closed
 */
int od_close(int od);

/**
 * @private
 * Generic lock opening method
 */
static inline int od_open(int fd, obj_type_t type, int obj_id)
{
	return od_openx(fd, type, obj_id, 0);
}

/**
 * public:
 * Open a lock, mark it used by the invoking thread
 * @param protocol Desired locking protocol
 * @param lock_id Name of the lock, user-specified numerical id
 * @param name_space Path to a shared file
 * @param config_param Any extra info needed by the protocol (like CPU for SRP
 * or PCP), may be NULL
 * @return Object descriptor for this lock
 */
int litmus_open_lock(obj_type_t protocol, int lock_id, const char* name_space,
		void *config_param);

/**
 * Obtain lock
 * @param od Object descriptor obtained by litmus_open_lock()
 * @return 0 iff the lock was opened successfully
 */
int litmus_lock(int od);
/**
 * Release lock
 * @param od Object descriptor obtained by litmus_open_lock()
 * @return 0 iff the lock was released successfully
 */
int litmus_unlock(int od);

/***** job control *****/
/**
 * @todo Doxygen
 */
int get_job_no(unsigned int* job_no);
/**
 * @todo Doxygen
 */
int wait_for_job_release(unsigned int job_no);
/**
 * Sleep until next period
 * @return 0 on success
 */
int sleep_next_period(void);

/**
 * Initialises real-time properties for the entire program
 * @return 0 on success
 */
int  init_litmus(void);
/**
 * Initialises real-time properties for current thread
 * @return 0 on success
 */
int  init_rt_thread(void);
/**
 * Cleans up real-time properties for the entire program
 */
void exit_litmus(void);

/*	per-task modes */
enum rt_task_mode_t {
	BACKGROUND_TASK = 0,
	LITMUS_RT_TASK  = 1
};
/**
 * Set the task mode for current thread
 * @param target_mode Desired mode, see enum rt_task_mode_t for valid values
 * @return 0 iff taskmode was set correctly
 */
int task_mode(int target_mode);

/**
 * @todo Document
 */
void show_rt_param(struct rt_task* tp);
/**
 * @todo Document
 */
task_class_t str2class(const char* str);

/**
 * Enter non-preemtpive section for current thread
 */
void enter_np(void);
/**
 * Exit non-preemtpive section for current thread
 */
void exit_np(void);
/**
 * Find out whether task should have preempted
 * @return 1 iff the task was requested to preempt while running non-preemptive
 */
int  requested_to_preempt(void);

/***** Task System support *****/
/**
 * Wait until task master releases all real-time tasks
 * @return 0 Iff task was successfully released
 */
int wait_for_ts_release(void);
/**
 * Release all tasks in the task system
 * @param delay Time to wait
 * @return Number of tasks released
 *
 * Used by a task master to release all threads after each of them has been
 * set up.
 */
int release_ts(lt_t *delay);
/**
 * Obtain the number of currently waiting tasks
 * @return The number of waiting tasks
 */
int get_nr_ts_release_waiters(void);
/**
 * @todo Document
 */
int read_litmus_stats(int *ready, int *total);

/**
 * Sleep for given time
 * @param timeout Sleep time in nanoseconds
 * @return 0 on success
 */
int lt_sleep(lt_t timeout);

/**
 * Sleep until the given point in time.
 * @param wake_up_time Point in time when to wake up (w.r.t. CLOCK_MONOTONIC,
 *                     in nanoseconds).
 */
void lt_sleep_until(lt_t wake_up_time);

/** Get the current time used by the LITMUS^RT scheduler.
 * This is just CLOCK_MONOTONIC and hence the same
 * as monotime(), but the result is given in nanoseconds
 * as a value of type lt_t.
 * @return CLOCK_MONOTONIC time in nanoseconds
 */
lt_t litmus_clock(void);

/**
 * Obtain CPU time consumed so far
 * @return CPU time in seconds
 */
double cputime(void);

/**
 * Obtain wall-clock time
 * @return Wall-clock time in seconds
 */
double wctime(void);

/**
 * Obtain CLOCK_MONOTONIC time
 * @return CLOCK_MONOTONIC time in seconds
 */
double monotime(void);

/**
 * Sleep until the given point in time.
 * @param wake_up_time Point in time when to wake up (w.r.t. CLOCK_MONOTONIC)
 */
void sleep_until_mono(double wake_up_time);

/**
 * Sleep until the given point in time.
 * @param wake_up_time Point in time when to wake up (w.r.t. CLOCK_REALTIME)
 */
void sleep_until_wc(double wake_up_time);

/***** semaphore allocation ******/
/**
 * Allocate a semaphore following the FMLP protocol
 * @param fd File descriptor to associate lock with
 * @param name Name of the lock, user-chosen integer
 * @return Object descriptor for given lock
 */
static inline int open_fmlp_sem(int fd, int name)
{
	return od_open(fd, FMLP_SEM, name);
}

/**
 * Allocate a semaphore following the SRP protocol
 * @param fd File descriptor to associate lock with
 * @param name Name of the lock, user-chosen integer
 * @return Object descriptor for given lock
 */
static inline int open_srp_sem(int fd, int name)
{
	return od_open(fd, SRP_SEM, name);
}

/**
 * Allocate a semaphore following the PCP protocol
 * @param fd File descriptor to associate lock with
 * @param name Name of the lock, user-chosen integer
 * @param cpu CPU to associate this lock with
 * @return Object descriptor for given lock
 */
static inline int open_pcp_sem(int fd, int name, int cpu)
{
	return od_openx(fd, PCP_SEM, name, &cpu);
}

/**
 * Allocate a semaphore following the MPCP protocol
 * @param fd File descriptor to associate lock with
 * @param name Name of the lock, user-chosen integer
 * @return Object descriptor for given lock
 */
static inline int open_mpcp_sem(int fd, int name)
{
	return od_open(fd, MPCP_SEM, name);
}

/**
 * Allocate a semaphore following the DPCP protocol
 * @param fd File descriptor to associate lock with
 * @param name Name of the lock, user-chosen integer
 * @param cpu CPU to associate this lock with
 * @return Object descriptor for given lock
 */
static inline int open_dpcp_sem(int fd, int name, int cpu)
{
	return od_openx(fd, DPCP_SEM, name, &cpu);
}


/**
 * Allocate a semaphore following the DFLP protocol
 * @param fd File descriptor to associate lock with
 * @param name Name of the lock, user-chosen integer
 * @param cpu CPU to associate this lock with
 * @return Object descriptor for given lock
 */
static inline int open_dflp_sem(int fd, int name, int cpu)
{
	return od_openx(fd, DFLP_SEM, name, &cpu);
}

/**
 * Get budget information from the scheduler (in nanoseconds).
 * @param expended pointer to time value in wich the total
 *        amount of already used-up budget will be stored.
 * @param remaining pointer to time value in wich the total
 *        amount of remaining budget will be stored.
 */

int get_current_budget(lt_t *expended, lt_t *remaining);

/**
 * Do nothing as a syscall
 * @param timestamp Cyclecount before calling
 * Can be used for syscall overhead measuring */
int null_call(cycles_t *timestamp);

/**
 * Get control page:
 * @return Pointer to the current tasks control page
 *
 * Atm it is used only by preemption migration overhead code,
 * but it is very general and can be used for different purposes
 */
struct control_page* get_ctrl_page(void);

#ifdef __cplusplus
}
#endif
#endif
