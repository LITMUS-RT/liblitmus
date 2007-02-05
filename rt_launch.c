#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

#include "litmus.h"

typedef struct {
	char * exec_path;
	char ** argv;
} startup_info_t;


int launch(void *task_info_p) {
	startup_info_t *info = (startup_info_t*) task_info_p;
	int ret;
	ret = execv(info->exec_path, info->argv);
	perror("execv failed");
	return ret;
}

void usage(char *error) {
	fprintf(stderr, "%s\nUsage: launch_rt [-c {hrt|srt|be}] [-p <cpu>]"
		"<wcet> <period> program arg1 arg2 ...\n",
		error);
	exit(1);
}

#define OPTSTR "p:c:"

int main(int argc, char** argv) 
{
	int ret;
	int wcet;
	int period;
	int cpu = 0;
	int opt;
	startup_info_t info;
	task_class_t class = RT_CLASS_HARD;
	
	while ((opt = getopt(argc, argv, OPTSTR)) != -1) {
		switch (opt) {
		case 'p':
			cpu = atoi(optarg);
			break;
		case 'c':
			class = str2class(optarg);
			if (class == -1)
				usage("Unknown task class.");
			break;

		case ':':
			usage("Argument missing.");
			break;
		case '?':
		default:
			usage("Unknown flag.");
			break;
		}
	}

	if (argc - optind < 3)
	{
		printf("argc: %d optind: %d\n", argc, optind);
		usage("Arguments missing.");       
	}
	wcet   = atoi(argv[optind + 0]);
	period = atoi(argv[optind + 1]);
	if (wcet <= 0)
		usage("The worst-case execution time must be a positive number.");
	if (period <= 0)
		usage("The period must be a positive number.");
	if (wcet > period) {
		usage("The worst-case execution time must not exceed the period.");
	}
	info.exec_path = argv[optind + 2];
	info.argv      = argv + optind + 2;
	ret = __create_rt_task(launch, &info, cpu, wcet, period, class);
	if (ret < 0) {
		perror("Could not create rt child process");
	}

	return 0;	
}
