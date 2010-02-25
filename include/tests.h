#ifndef TESTS_H
#define TESTS_H

#include <stdio.h>
#include <stdlib.h>
#include <errno.h>

#define fail(fmt, args...)						\
	do {								\
		fprintf(stderr, "\n!! TEST FAILURE " fmt "\n   at %s:%d (%s)\n", \
			## args, __FILE__, __LINE__, __FUNCTION__);	\
		fflush(stderr);						\
		exit(200);						\
	} while (0)

#define ASSERT(predicate)					\
	do {							\
		if (!(predicate))				\
			fail("%s", #predicate);			\
	} while (0)

#define SYSCALL(call)					\
	do {						\
		if ((call) < 0)				\
			fail("%s, %m", #call);		\
	} while (0)

#define SYSCALL_FAILS(expected, call)					\
	do {								\
		if ((call) == 0 || errno != (expected))			\
			fail("%s, %m (expected: %s)", #call, #expected); \
	} while (0)


typedef void (*testfun_t)(void);

struct testcase {
	testfun_t   function;
	const char* description;
};

struct testsuite {
	const char* plugin;
	int* testcases;
	int  num_cases;
};

#define TESTCASE(function, plugins, description) void test_ ## function (void)

#endif
