#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <limits.h>
#include <signal.h>

#include "litmus.h"

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
		"\nwhere:"
		"\n\t <SPORADIC OPTIONS> = "
		"[-c {hrt|srt|be}] [-p <cpu>]\n"
		"\nExamples:"
		"\n\trt_launch -p 2 10 100 cpu_job"
		"\n\t  => Launch cpu_job a hard real-time task with "
		"\n\t     period 100ms and weight 0.1 on CPU 2.\n"
		"\n\n",
		error);
	exit(1);
}


#define OPTSTR "p:c:v"

int main(int argc, char** argv) 
{
	int ret;
	lt_t wcet;
	lt_t period;
	int cpu = 0;
	int opt;
	int verbose = 0;
	startup_info_t info;
	task_class_t class = RT_CLASS_HARD;

	while ((opt = getopt(argc, argv, OPTSTR)) != -1) {
		switch (opt) {
		case 'v':
			verbose = 1;
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

	signal(SIGUSR1, SIG_IGN);

	if (argc - optind < 3)
		usage("Arguments missing.");       
	wcet   = ms2lt(atoi(argv[optind + 0]));
	period = ms2lt(atoi(argv[optind + 1]));
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

	
	if (ret < 0) {
		perror("Could not create rt child process");
		return 2;	
	} else if (verbose)
		printf("%d\n", ret);

	return 0;	
}
