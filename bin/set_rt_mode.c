#include <stdio.h>
#include <strings.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>

#include "litmus.h"


void die(const char* str)
{
	fprintf(stderr, "%s\n%s\n", str, 
		"Usage: set_rt_mode  [-v] {on|off}");
	exit(127);
}

int main(int argc, char** argv) 
{
	int ret;
	int verbose = 0;
	char* cmd = argv[1];

	if (argc == 3 && !strcmp(argv[1], "-v")) {
		verbose = 1;
		cmd = argv[2];
	} 
	if (argc == 2 || verbose) {	
		if (!strcmp(cmd, "on")) {
			if (verbose)
				printf("Enabling real-time mode.\n");
			ret = set_rt_mode(MODE_RT_RUN);			
			if (ret && verbose) 
				perror("set_rt_mode failed");
			exit(ret ? errno : 0);
		} else if (!strcmp(cmd, "off")) {
			if (verbose)
				printf("Disabling real-time mode.\n");
			ret = set_rt_mode(MODE_NON_RT);	
			if (ret && verbose) 
				perror("set_rt_mode failed");
			exit(ret ? errno : 0);		
		} 
	}
	die("Bad arguments.");
	return 0;	
}
