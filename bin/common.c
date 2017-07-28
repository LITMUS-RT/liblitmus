#include <stdio.h>
#include <stdlib.h>
#include <limits.h>
#include <string.h>
#include <errno.h>

#include "common.h"

void bail_out(const char* msg)
{
	perror(msg);
	exit(-1 * errno);
}

int str2int(const char* arg, int *failure_flag)
{
	long val;
	char *end;

	val = strtol(arg, &end, 10);
	/* upon successful conversion, must point to string end */
	if (failure_flag)
		*failure_flag = *arg == '\0' || *end != '\0' ||
		                val > INT_MAX || val < INT_MIN;
	return (int) val;
}

double str2double(const char* arg, int *failure_flag)
{
	double val;
	char *end;

	val = strtod(arg, &end);
	/* upon successful conversion, must point to string end */
	if (failure_flag)
		*failure_flag = *arg == '\0' || *end != '\0';
	return val;
}

char* strsplit(char split_char, char *str)
{
	char *found = strrchr(str, split_char);
	if (found) {
		/* terminate first string and move to next char */
		*found++ = '\0';
	}
	return found;
}
