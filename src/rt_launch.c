#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <limits.h>

#include "litmus.h"
#include "adaptive.h"

typedef struct {
	char * exec_path;
	char ** argv;
} startup_info_t;


int launch(void *task_info_p) {
	startup_info_t *info = (startup_info_t*) task_info_p;
	int ret;
	ret = execvp(info->exec_path, info->argv);
	perror("execv failed");
	return ret;
}

void usage(char *error) {
	fprintf(stderr, "%s\nUsage: launch_rt [-c {hrt|srt|be}] [-p <cpu>]"
		"{<wcet> <period>|{-a wcet/period/utility}+}"
		" program arg1 arg2 ...\n",
		error);
	exit(1);
}

/* argument format should be wcet/period/utility */
static int parse_service_level(service_level_t* level, char* str) 
{
	char *wcet, *period, *utility;
	double u;
	wcet    = strtok(str, "/");
	period  = strtok(NULL, "/");
        utility = strtok(NULL, "/");
	str     = strtok(NULL, "/");

	if (str || !utility || !period || !wcet)
		return 0;

	level->exec_cost = atol(wcet);
	level->period    = atol(period);
	u = atof(utility);

	if (level->exec_cost == 0 || level->period < level->exec_cost ||
	    u <= 0.0 || u > 1.0)
		return 0;

	level->utility = (unsigned long)  ULONG_MAX * u;
	return 1;
}

#define OPTSTR "p:c:a:"

int main(int argc, char** argv) 
{
	int ret;
	int wcet;
	int period;
	int cpu = 0;
	int opt;
	startup_info_t info;
	task_class_t class = RT_CLASS_HARD;

	int adaptive = 0;
	unsigned int level = 0;
	service_level_t slevel[MAX_SERVICE_LEVELS];
	
	while ((opt = getopt(argc, argv, OPTSTR)) != -1) {
		switch (opt) {
		case 'a':						
			adaptive = 1;
			if (level == MAX_SERVICE_LEVELS)
				usage("Too many service levels.");
			if (!parse_service_level(slevel + level++, optarg))
				usage("Bad service level.");			
			break;
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

	if (!adaptive) {
		if (argc - optind < 3)
			usage("Arguments missing.");       
		wcet   = atoi(argv[optind + 0]);
		period = atoi(argv[optind + 1]);
		if (wcet <= 0)
		usage("The worst-case execution time must be a "
		      "positive number.");
		if (period <= 0)
			usage("The period must be a positive number.");
		if (wcet > period) {
			usage("The worst-case execution time must not "
			      "exceed the period.");
		}
		info.exec_path = argv[optind + 2];
		info.argv      = argv + optind + 2;
		ret = __create_rt_task(launch, &info, cpu, wcet, period, class);
	} else {
		if (argc == optind)
			usage("Arguments missing.");       
		info.exec_path = argv[optind];
		info.argv      = argv + optind;
		ret = create_adaptive_rt_task(launch, &info, level, slevel);
	}

	
	if (ret < 0) {
		perror("Could not create rt child process");
		return 2;
	}

	return 0;	
}
