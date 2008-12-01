#include <sys/time.h>

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <time.h>


#include "litmus.h"
#include "common.h"

void usage(char *error) {
	fprintf(stderr, "Error: %s\n", error);
	fprintf(stderr, "Usage: rt_spin [-w] [-p PARTITION] [-c CLASS] WCET PERIOD DURATION\n");
	exit(1);
}

static double wctime()
{
	struct timeval tv;
	gettimeofday(&tv, NULL);
	return (tv.tv_sec + 1E-6 * tv.tv_usec);
}

#define OPTSTR "p:c:w"

int main(int argc, char** argv) 
{
	int i;
	int ret;
	lt_t wcet;
	lt_t period;
	int migrate = 0;
	int cpu = 0;
	int opt;
	int wait = 0;
	double duration, start;
	task_class_t class = RT_CLASS_HARD;

	while ((opt = getopt(argc, argv, OPTSTR)) != -1) {
		switch (opt) {
		case 'w':
			wait = 1;
			break;
		case 'p':
			cpu = atoi(optarg);
			migrate = 1;
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

	if (argc - optind < 3)
		usage("Arguments missing.");       
	wcet     = atoi(argv[optind + 0]);
	period   = atoi(argv[optind + 1]);
	duration = atof(argv[optind + 2]);
	if (wcet <= 0)
		usage("The worst-case execution time must be a "
		      "positive number.");
	if (period <= 0)
		usage("The period must be a positive number.");
	if (wcet > period) {
		usage("The worst-case execution time must not "
		      "exceed the period.");
	}
	if (migrate) {
		ret = be_migrate_to(cpu);
		if (ret < 0)
			bail_out("could not migrate to target partition");
	}
	
//	printf("wcet: %llu\nperiod: %llu\n", wcet, period);
	ret = sporadic_task(wcet, period, 0, cpu, class, migrate);
	
	if (ret < 0)
		bail_out("could not become rt tasks.");

//	ret = init_litmus();
//	if (ret < 0)
//		perror("init_litmus()");

	ret = task_mode(LITMUS_RT_TASK);
	if (ret != 0)
		bail_out("task_mode()");

	if (wait) {
		ret = wait_for_ts_release();
		if (ret != 0)
			bail_out("wait_for_ts_release()");
	}
	start = wctime();

	while (start + duration > wctime()) {
		for (i = 0; i < 500000; i++);
	}

	return 0;
}
