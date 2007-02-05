#include <sys/types.h>
#include <stdio.h>

#include "litmus.h"

int main(int argc, char** argv) {
	spolicy scheduler = sched_getpolicy();	
	printf("Current scheduler: %s (%d)\n", get_scheduler_name(scheduler), 
	       scheduler);
	return 0;
}
