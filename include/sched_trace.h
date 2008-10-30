#ifndef __SCHED_TRACE_H_
#define __SCHED_TRACE_H_

#include <linux/types.h>

typedef __u8  u8;
typedef __u32 u32;
typedef __u16 u16;
typedef __u64 u64;

#include <litmus/sched_trace.h>

const char* event2name(unsigned int id);
u64 event_time(struct st_event_record* rec);

void print_event(struct st_event_record *rec);
void print_all(struct st_event_record *rec, unsigned int count);


#endif
