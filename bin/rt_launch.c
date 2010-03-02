#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <limits.h>
#include <signal.h>

#include "litmus.h"
#include "common.h"

typedef struct {
	int  wait;
	char * exec_path;
	char ** argv;
} startup_info_t;


int launch(void *task_info_p) {
	startup_info_t *info = (startup_info_t*) task_info_p;
	int ret;
	if (info->wait) {
		ret = wait_for_ts_release();
		if (ret != 0)
			perror("wait_for_ts_release()");
	}
	ret = execvp(info->exec_path, info->argv);
	perror("execv failed");
	return ret;
}

void usage(char *error) {
	fprintf(stderr, "%s\nUsage: rt_launch [-w][-v][-p cpu][-c hrt | srt | be] wcet period program [arg1 arg2 ...]\n"
			"\t-w\tSynchronous release\n"
			"\t-v\tVerbose\n"
			"\t-p\tcpu (or initial cpu)\n"
			"\t-c\tClass\n"
			"\twcet, period in ms\n"
			"\tprogram to be launched\n",
			error);
	exit(1);
}


#define OPTSTR "p:c:vw"

int main(int argc, char** argv) 
{
	int ret;
	lt_t wcet;
	lt_t period;
	int migrate = 0;
	int cpu = 0;
	int opt;
	int verbose = 0;
	int wait = 0;
	startup_info_t info;
	task_class_t class = RT_CLASS_HARD;

	while ((opt = getopt(argc, argv, OPTSTR)) != -1) {
		switch (opt) {
		case 'w':
			wait = 1;
			break;
		case 'v':
			verbose = 1;
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
	info.wait      = wait;
	if (migrate) {
		ret = be_migrate_to(cpu);
		if (ret < 0)
			bail_out("could not migrate to target partition");
	}
	ret = __create_rt_task(launch, &info, cpu, wcet, period, class);

	
	if (ret < 0)
		bail_out("could not create rt child process");
	else if (verbose)
		printf("%d\n", ret);

	return 0;	
}
