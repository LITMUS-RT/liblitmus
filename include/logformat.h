#ifndef LOGFORMAT_H
#define LOGFORMAT_H

#include <linux/types.h>

#include "litmus.h"

typedef __u8  u8;
typedef __u32 u32;
typedef __u16 u16;

typedef enum {
	ST_INVOCATION           = 0,
	ST_ARRIVAL		= 1,
	ST_DEPARTURE		= 2,
	ST_PREEMPTION		= 3,
	ST_SCHEDULED		= 4,
	ST_JOB_RELEASE		= 5,
	ST_JOB_COMPLETION	= 6,
	ST_CAPACITY_RELEASE     = 7,
	ST_CAPACITY_ALLOCATION  = 8,
	ST_LAST_TYPE            = 8
} trace_type_t;

typedef struct {
	trace_type_t		trace:8;
	unsigned long long	timestamp;
} trace_header_t;


typedef struct {
	unsigned int		is_rt:1;
	unsigned int            is_server:1;
	task_class_t		cls:4;
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


typedef union {
	trace_header_t          header;
       	invocation_record_t 	invocation;
	arrival_record_t	arrival;
	preemption_record_t	preemption;
	scheduled_record_t	scheduled;

	release_record_t	release;
	completion_record_t	completion;	

	cap_release_record_t	cap_release;
	cap_allocation_record_t cap_alloc;
} trace_record_t;



#endif
