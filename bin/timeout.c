#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

#include "litmus.h"

int main(int argc, char** argv) 
{
	int timeout;
	rt_param_t my_param;

	if (argc != 2) 
	{
		printf("Usage: %s <timeout in seconds>\n", argv[0]);
		exit(1);
	}
	timeout = atoi(argv[1]);
	if (timeout <= 0) {
		printf("Timeout must be a positive number.\n");
		exit(1);
	}

	/*		printf("This is the kill real-time mode task.\n" 
	       "Expected end of real %u seconds.\n", timeout);
	*/
	/*	get_rt_task_param(getpid(), &my_param);
	show_rt_param(&my_param);
	*/
	
	sleep(timeout);


	set_rt_mode(MODE_NON_RT);
	printf("End of real-time mode.\n");
	return 0;
}
