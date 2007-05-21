#include <sys/types.h>
#include <stdio.h>

#include "litmus.h"

int main(int argc, char** argv) {
	spolicy scheduler = sched_getpolicy();	
	printf("%s\n", get_scheduler_name(scheduler));
	return 0;
}
