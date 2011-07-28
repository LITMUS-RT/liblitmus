#ifndef INTERNAL_H
#define INTERNAL_H

/* low level operations, not intended for API use */

/* prepare a real-time task */
typedef int (*rt_setup_fn_t)(int pid, void* arg);
int __launch_rt_task(rt_fn_t rt_prog, void *rt_arg,
		     rt_setup_fn_t setup, void* setup_arg);

#define check(str)	 \
	if (ret == -1) { \
		perror(str); \
		fprintf(stderr,	\
			"Warning: Could not initialize LITMUS^RT, " \
			"%s failed.\n",	str			    \
			); \
	}


/* taken from the kernel */

#define likely(x)   __builtin_expect((x), 1)
#define unlikely(x) __builtin_expect((x), 0)

#define BUILD_BUG_ON_ZERO(e) (sizeof(struct { int:-!!(e); }))
#define BUILD_BUG_ON(condition) ((void)BUILD_BUG_ON_ZERO(condition))

#endif

