#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <limits.h>
#include <signal.h>

#include "litmus.h"
#include "common.h"

const char *usage_msg =
	"Usage: rt_launch OPTIONS budget period -- program [arg1 arg2 ...]\n"
	"\n"
	"Required arguments:\n"
	"    budget, period    reservation parameters (in ms)\n"
	"    program           path to the binary to be launched\n"
	"\n"
	"Options:\n"
	"    -c be|srt|hrt     task class (best-effort, soft real-time, hard real-time)\n"
	"    -d DEADLINE       relative deadline, equal to the period by default (in ms)\n"
	"    -e                turn off budget enforcement (DANGEROUS: can result in lockup)\n"
	"    -h                show this help message\n"
	"    -o OFFSET         offset (also known as phase), zero by default (in ms)\n"
	"    -p CPU            partition or cluster to assign this task to\n"
	"    -q PRIORITY       priority to use (ignored by EDF plugins, highest=1, lowest=511)\n"
	"    -r VCPU           virtual CPU or reservation to attach to (irrelevant to most plugins)\n"
	"    -R                create sporadic reservation for task (with VCPU=PID)\n"
	"    -v                verbose (prints PID)\n"
	"    -w                wait for synchronous release\n"
	"\n";

void usage(char *error) {
	fprintf(stderr, "%s\n\n%s", error, usage_msg);
	exit(1);
}

#define OPTSTR "wp:q:c:er:o:d:vhR"

int main(int argc, char** argv)
{
	int ret, opt;
	lt_t wcet;
	lt_t period;
	lt_t offset;
	lt_t deadline;
	double wcet_ms, period_ms, offset_ms, deadline_ms;
	unsigned int priority;
	int migrate;
	int cluster;
	int reservation;
	int wait;
	int want_enforcement;
	int verbose;
	int create_reservation;
	task_class_t class;
	struct rt_task param;

	/* Reasonable defaults */
	verbose = 0;
	offset_ms = 0;
	deadline_ms = 0;
	priority = LITMUS_NO_PRIORITY;
	want_enforcement = 1; /* safety: default to enforcement */
	wait = 0;  /* don't wait for task system release */
	class = RT_CLASS_SOFT; /* ignored by most plugins */
	migrate = 0; /* assume global unless -p is specified */
	cluster = -1; /* where to migrate to, invalid by default */
	reservation = -1; /* set if task should attach to virtual CPU */
	create_reservation = 0;

	while ((opt = getopt(argc, argv, OPTSTR)) != -1) {
		switch (opt) {
		case 'w':
			wait = 1;
			break;
		case 'p':
			cluster = atoi(optarg);
			migrate = 1;
			break;
		case 'q':
			priority = atoi(optarg);
			if (!litmus_is_valid_fixed_prio(priority))
				usage("Invalid priority.");
			break;
		case 'c':
			class = str2class(optarg);
			if (class == -1)
				usage("Unknown task class.");
			break;
		case 'e':
			want_enforcement = 0;
			break;
		case 'r':
			reservation = atoi(optarg);
			break;
		case 'R':
			create_reservation = 1;
			reservation = getpid();
			break;
		case 'o':
			offset_ms = atof(optarg);
			break;
		case 'd':
			deadline_ms = atof(optarg);
			if (!deadline_ms || deadline_ms < 0) {
				usage("The relative deadline must be a positive"
					" number.");
			}
			break;
		case 'v':
			verbose = 1;
			break;
		case 'h':
			usage("rt_launch: Run an arbitrary binary as a LITMUS^RT real-time task.");
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

	wcet_ms   = atof(argv[optind + 0]);
	period_ms = atof(argv[optind + 1]);

	wcet   = ms2ns(wcet_ms);
	period = ms2ns(period_ms);
	offset = ms2ns(offset_ms);
	deadline = ms2ns(deadline_ms);
	if (wcet <= 0)
		usage("The worst-case execution time must be a "
				"positive number.");

	if (offset_ms < 0)
		usage("The synchronous release delay must be a "
				"non-negative number.");

	if (period <= 0)
		usage("The period must be a positive number.");
	if (wcet > period) {
		usage("The worst-case execution time must not "
				"exceed the period.");
	}

	if (migrate) {
		ret = be_migrate_to_domain(cluster);
		if (ret < 0)
			bail_out("could not migrate to target partition or cluster.");
	}

	init_rt_task_param(&param);
	param.exec_cost = wcet;
	param.period = period;
	param.phase = offset;
	param.relative_deadline = deadline;
	param.priority = priority == LITMUS_NO_PRIORITY ? LITMUS_LOWEST_PRIORITY : priority;
	param.cls = class;
	param.budget_policy = (want_enforcement) ?
			PRECISE_ENFORCEMENT : NO_ENFORCEMENT;
	if (migrate) {
		if (reservation >= 0)
			param.cpu = reservation;
		else
			param.cpu = domain_to_first_cpu(cluster);
	}
	ret = set_rt_task_param(gettid(), &param);
	if (ret < 0)
		bail_out("could not setup rt task params");

	if (create_reservation) {
		struct reservation_config config;
		memset(&config, 0, sizeof(config));
		config.id = gettid();
		config.cpu = domain_to_first_cpu(cluster);
		config.priority = priority;
		config.polling_params.budget = wcet;
		config.polling_params.period = period;
		config.polling_params.offset = offset;
		config.polling_params.relative_deadline = deadline;
		ret = reservation_create(SPORADIC_POLLING, &config);
		if (ret < 0)
			bail_out("failed to create reservation");
	}

	init_litmus();

	ret = task_mode(LITMUS_RT_TASK);
	if (ret != 0)
		bail_out("could not become RT task");

	if (verbose)
		printf("%d\n", gettid());

	if (wait) {
		ret = wait_for_ts_release();
		if (ret != 0)
			bail_out("wait_for_ts_release()");
	}

	execvp(argv[optind + 2], argv + optind + 2);
	perror("execv failed");
	return 1;
}
