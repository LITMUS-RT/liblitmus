#ifndef __SCHED_TRACE_H_
#define __SCHED_TRACE_H_

#include <linux/types.h>

typedef __u8  u8;
typedef __u32 u32;
typedef __u16 u16;

typedef enum {
	ST_INVOCATION           =  0,
	ST_ARRIVAL		=  1,
	ST_DEPARTURE		=  2,
	ST_PREEMPTION		=  3,
	ST_SCHEDULED		=  4,
	ST_JOB_RELEASE		=  5,
	ST_JOB_COMPLETION	=  6,
	ST_CAPACITY_RELEASE     =  7,
	ST_CAPACITY_ALLOCATION  =  8,

	ST_MAX
} trace_type_t;


typedef struct {
	trace_type_t		trace:8;
	unsigned int		size:24;
	unsigned long long	timestamp;	
} trace_header_t;

typedef struct {
	unsigned int		is_rt:1;
	unsigned int            is_server:1;
	task_class_t		class:4;
	unsigned int            budget:24;
	u32			deadline;

	pid_t			pid;
} task_info_t;

typedef struct {
	trace_header_t 		header;
        unsigned long           flags;
} invocation_record_t;

typedef struct {
	trace_header_t 		header;
	task_info_t		task;
} arrival_record_t;

typedef struct {
	trace_header_t 		header;
	task_info_t		task;
} departure_record_t;

typedef struct {
	trace_header_t 		header;
	task_info_t		task;
	task_info_t		by;
} preemption_record_t;

typedef struct {
	trace_header_t 		header;
	task_info_t		task;
} scheduled_record_t;

typedef struct {
	trace_header_t 		header;
	task_info_t		task;
	u16   		        period;
	u16            		wcet;
} release_record_t;

typedef struct {
	trace_header_t 		header;
	task_info_t		task;
	u16   		        period;
	u16            		wcet;
	int			tardiness;
	unsigned int		job_no;
} completion_record_t;

typedef struct {
	trace_header_t 		header;
	task_info_t		task;
} cap_release_record_t;

typedef struct {
	trace_header_t 		header;
	task_info_t		task;
	u16 			budget;
	u32                     deadline;
	pid_t			donor;
} cap_allocation_record_t;





enum {
	INVALID_HEADER = 1
};

typedef int (*record_callback_fn)(trace_header_t* hdr);

typedef struct {
	record_callback_fn handler[ST_MAX];
} record_callback_t;

#define init_record_callback(a) memset(a, 0, sizeof(record_callback_t))
#define set_callback(type, fn, a)    do {(a)->handler[type] = fn; } while (0);

int walk_sched_trace(void* start, void* end, record_callback_t *cb);
int walk_sched_traces_ordered(void** start, void** end, unsigned int count,
			      record_callback_t *cb);

int walk_sched_trace_file(const char* name, int keep_mapped, 
			  record_callback_t *cb);
int walk_sched_trace_files_ordered(const char** names, unsigned int count, 
				   int keep_mapped, record_callback_t *cb);
#endif
