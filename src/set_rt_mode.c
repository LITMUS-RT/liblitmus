#include <stdio.h>
#include <strings.h>
#include <stdlib.h>
#include "litmus.h"


int main(int argc, char** argv) 
{
	if (argc == 2) {
		if (!strcmp(argv[1], "on")) {
			printf("Enabling real-time mode.\n");
			set_rt_mode(MODE_RT_RUN);			
			exit(0);
		} else if (!strcmp(argv[1], "off")) {
			printf("Disabling real-time mode.\n");
			set_rt_mode(MODE_NON_RT);			
			exit(0);
		} 
	}
	printf("Usage: %s {on|off}\n", argv[0]);
	return 1;	
}
