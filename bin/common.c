#include <stdio.h>
#include <stdlib.h>
#include <errno.h>

#include "common.h"

void bail_out(const char* msg)
{
	perror(msg);
	exit(-1 * errno);
}
