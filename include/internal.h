#ifndef INTERNAL_H
#define INTERNAL_H

/* low level operations, not intended for API use */

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

#define offsetof(s, x) __builtin_offsetof(s, x)

#define BUILD_BUG_ON_ZERO(e) (sizeof(struct { int:-!!(e); }))
#define BUILD_BUG_ON(condition) ((void)BUILD_BUG_ON_ZERO(condition))

/* I/O convenience function */
ssize_t read_file(const char* fname, void* buf, size_t maxlen);

#endif

