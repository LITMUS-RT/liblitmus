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
	fprintf(stderr, "%s\nUsage: \nlaunch_rt supports one of two modes:\n"
		"\n\tlaunch_rt <SPORADIC OPTIONS> <wcet> <period> program arg1 arg2 ...\n"
		"\n\tlaunch_rt <ADAPTIVE OPTIONS> program arg1 arg2 ...\n"
		"\nwhere:"
		"\n\t <SPORADIC OPTIONS> = "
		"[-c {hrt|srt|be}] [-p <cpu>]\n"
		"\n\t <ADAPTIVE OPTIONS> = "
		"(-a weight/period/utility)+\n"
		"\nExamples:"
		"\n\trt_launch -p 2 10 100 cpu_job"
		"\n\t  => Launch cpu_job a hard real-time task with "
		"\n\t     period 100 and weight 0.1 on CPU 2.\n"
		"\n\trt_launch -a 0.1/100/0.4 -a 0.2/75/0.5 adaptive_job"
		"\n\t  => Launch adaptive_job with two service levels"
		"\n\n",
		error);
	exit(1);
}

/* argument format should be weight/period/utility */
static int parse_service_level(service_level_t* level, char* str) 
{
	char *weight, *period, *utility;
	double u, w;
	weight    = strtok(str, "/");
	period  = strtok(NULL, "/");
        utility = strtok(NULL, "/");
	str     = strtok(NULL, "/");

	if (str || !utility || !period || !weight)
		return 0;

	w = atof(weight);
	u = atof(utility);
	level->weight = f2fp(w);
	level->period = atol(period);
	level->value   = f2fp(u);
	
	if (level->period == 0  ||
	    u <= 0.0 || u > 1.0 || w <= 0.0 || w > 1.0)
		return 0;
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
			usage("Bad argument.");
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
