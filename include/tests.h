/**
 * @file tests.h
 * Structs and macro's for use in unit test cases
 */

#ifndef TESTS_H
#define TESTS_H

#include <stdio.h>
#include <stdlib.h>
#include <errno.h>

/**
 * @private
 * Print a failure message and exit
 * @param fmt Error string
 * @param args... Parameters for error string
 */
#define fail(fmt, args...)						\
	do {								\
		fprintf(stderr, "\n!! TEST FAILURE " fmt		\
			"\n   at %s:%d (%s)"				\
			"\n   in task PID=%d\n",			\
			## args, __FILE__, __LINE__, __FUNCTION__,	\
			getpid());					\
		fflush(stderr);						\
		exit(200);						\
	} while (0)

/**
 * Assert given predicate, print error if it doesn't hold
 * @param predicate Predicate that must hold
 */
#define ASSERT(predicate)					\
	do {							\
		if (!(predicate))				\
			fail("%s", #predicate);			\
	} while (0)

/**
 * Do and trace a syscall
 * @param call Syscall to execute
 */
#define SYSCALL(call)							\
	do {								\
		int __test_ret = (call);				\
		if (__test_ret < 0)					\
			fail("%s -> %d, %m", #call, __test_ret);	\
	} while (0)

/**
 * Do and trace a syscall that is expected to fail
 * @param expected Expected error code
 * @param call Syscall to execute
 */
#define SYSCALL_FAILS(expected, call)					\
	do {								\
		int __test_ret = (call);				\
		if (__test_ret == 0 || errno != (expected))		\
			fail("%s -> %d, %m (expected: %s)",		\
			     #call, __test_ret, #expected);		\
	} while (0)

/**
 * Function prototype for a single test case
 */
typedef void (*testfun_t)(void);

/**
 * Test case
 */
struct testcase {
	testfun_t   function; /**< Function-pointer to test-case */
	const char* description; /**< Description of test-case */
};

/**
 * Suite containing several test cases
 */
struct testsuite {
	const char* plugin;	/**< Lock scheduling plugin to use */
	int* testcases; 	/**< Pointer to array of test-cases
							 @todo why not struct testcase?*/
	int  num_cases;		/**< Number of test cases in this suite */
};

/**
 * Function descriptor for test case
 * @param function Test name
 * @param plugins Set of lock scheduling plugins this test case is applicable
 * for, separated by |
 * @param description Textual description of the test case
 *
 * Testcases defined with this macro will get picked up by a python test suite
 * generator
 */
#define TESTCASE(function, plugins, description) void test_ ## function (void)

/**
 * Fork given function body as separate thread
 * @param code Function body
 */
#define FORK_TASK(code) ({int __pid = fork(); if (__pid == 0) {code; exit(0);}; __pid;})

#endif
