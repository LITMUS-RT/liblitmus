/**
 * @file common.h
 * Common miscellaneous functions
 */

#ifndef COMMON_H
#define COMMON_H

/**
 * End the current task with a message
 * @param msg Message to output before bailing
 */
void bail_out(const char* msg);

/**
 * Convert the given argument to an integer; set the flag if it fails.
 * @param arg String to convert to integer (must not be NULL).
 * @param failure_flag pointer to flag that will be set if conversion failed (may be NULL).
 * @return integer value of arg
 */
int str2int(const char* arg, int *failure_flag);

/**
 * Convert the given argument to a double; set the flag if it fails.
 * @param arg String to convert to double (must not be NULL).
 * @param failure_flag pointer to flag that will be set if conversion failed (may be NULL).
 * @return integer value of arg
 */
double str2double(const char* arg, int *failure_flag);


/* the following macros assume that there is a no-return function called usage() */

#define want_int_min(arg, min, msg) ({	\
	int __val;			\
	int __fail;			\
	__val = str2int(arg, &__fail);	\
	if (__fail || __val < min)	\
		usage(msg);		\
	__val;})

#define want_double_min(arg, min, msg) ({	\
	double __val;				\
	int __fail;				\
	__val = str2double(arg, &__fail);	\
	if (__fail || __val < min)		\
		usage(msg);			\
	__val;})

#define want_non_negative_int(arg, name) \
	want_int_min(arg, 0, "option " name " requires a non-negative integer argument")

#define want_positive_int(arg, name) \
	want_int_min(arg, 1, "option " name " requires a positive integer argument")

#define want_non_negative_double(arg, name) \
	want_double_min(arg, 0, "option " name " requires a non-negative argument")

#define want_positive_double(arg, name) ({ \
	double __val = want_double_min(arg, 0, "option " name " requires a positive argument"); \
	if (!__val) \
		usage("option " name " requires a positive argument"); \
	__val; })

char* strsplit(char split_char, char *str);

#endif
