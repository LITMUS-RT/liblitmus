#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <limits.h>
#include <signal.h>

#include <sched.h>

#include "litmus.h"
#include "common.h"

const char *usage_msg =
	"Usage: resctl OPTIONS [INTERVAL-START,INTERVAL-END]*\n"
	"    -n ID             create new reservation with id ID\n"
	"    -a PID            attach already-running process PID to reservation\n"
	"    -r ID             specify which reservation to attach to (not needed with -n)\n"
	"    -t TYPE           type of reservation (polling-periodic, polling-sporadic, table-driven)\n"
	"    -c CPU            physical partition or cluster to assign to\n"
	"    -b BUDGET         polling reservation budget (in ms, default: 10ms)\n"
	"    -p PERIOD         polling reservation period (in ms, default: 100ms)\n"
	"    -d DEADLINE       relative deadline, implicit by default (in ms)\n"
	"    -o OFFSET         offset (also known as phase), zero by default (in ms)\n"
	"    -q PRIORITY       priority to use (EDF by default, max=0)\n"
	"    -m MAJOR-CYCLE    major cycle length (in ms, for table-driven reservations) \n"
	"\n";

void usage(char *error) {
	fprintf(stderr, "%s\n%s", error, usage_msg);
	exit(1);
}


static void attach_task(int attach_pid, struct reservation_config *config)
{
	int ret;
	struct rt_task param;
	struct sched_param linux_param;

	ret = be_migrate_thread_to_cpu(attach_pid, config->cpu);
	if (ret < 0) {
		fprintf(stderr, "failed to migrate task %d to CPU %d\n",
			attach_pid, config->cpu);
		exit(4);
	}

	init_rt_task_param(&param);
	/* dummy values */
	param.exec_cost = ms2ns(100);
	param.period    = ms2ns(100);
	/* specify reservation as "virtual" CPU */
	param.cpu       = config->id;

	ret = set_rt_task_param(attach_pid, &param);
	if (ret < 0) {
		fprintf(stderr, "failed to set RT task parameters for task %d (%m)\n",
			attach_pid);
		exit(2);
	}

	linux_param.sched_priority = 0;
	ret = sched_setscheduler(attach_pid, SCHED_LITMUS, &linux_param);
	if (ret < 0) {
		fprintf(stderr, "failed to transition task %d to LITMUS^RT class (%m)\n",
			attach_pid);
		exit(3);
	}
}

static struct lt_interval* parse_td_intervals(int argc, char** argv,
	unsigned int *num_intervals, lt_t major_cycle)
{
	int i, matched;
	struct lt_interval *slots = malloc(sizeof(slots[0]) * argc);
	double start, end;

	*num_intervals = 0;
	for (i = 0; i < argc; i++) {
		matched = sscanf(argv[i], "[%lf,%lf]", &start, &end);
		if (matched != 2) {
			fprintf(stderr, "could not parse '%s' as interval\n", argv[i]);
			exit(5);
		}
		if (start < 0) {
			fprintf(stderr, "interval %s: must not start before zero\n", argv[i]);
			exit(5);
		}
		if (end <= start) {
			fprintf(stderr, "interval %s: end before start\n", argv[i]);
			exit(5);
		}

		slots[i].start = ms2ns(start);
		slots[i].end   = ms2ns(end);

		if (i > 0 && slots[i - 1].end >= slots[i].start) {
			fprintf(stderr, "interval %s: overlaps with previous interval\n", argv[i]);
			exit(5);
		}

		if (slots[i].end >= major_cycle) {
			fprintf(stderr, "interval %s: exceeds major cycle length\n", argv[i]);
			exit(5);
		}

		(*num_intervals)++;
	}

	return slots;
}

#define OPTSTR "n:a:r:t:c:b:p:d:o:q:m:h"

int main(int argc, char** argv)
{
	int ret, opt;
	double budget_ms, period_ms, offset_ms, deadline_ms, major_cycle_ms;
	int create_new = 0;
	int attach_pid = 0;
	int res_type = SPORADIC_POLLING;

	struct reservation_config config;

	/* Reasonable defaults */
	offset_ms      =    0;
	deadline_ms    =    0;
	budget_ms      =   10;
	period_ms      =  100;
	major_cycle_ms = 1000;

	config.id = 0;
	config.priority = LITMUS_NO_PRIORITY; /* use EDF by default */
	config.cpu = 0;

	while ((opt = getopt(argc, argv, OPTSTR)) != -1) {
		switch (opt) {
		case 'b':
			budget_ms = atof(optarg);
			break;
		case 'p':
			period_ms = atof(optarg);
			break;
		case 'd':
			deadline_ms = atof(optarg);
			if (deadline_ms <= 0) {
				usage("The relative deadline must be a positive"
					" number.");
			}
			break;
		case 'o':
			offset_ms = atof(optarg);
			break;
		case 'm':
			major_cycle_ms = atof(optarg);
			break;

		case 'q':
			config.priority = atoi(optarg);
			break;
		case 'c':
			config.cpu = atoi(optarg);
			break;

		case 'n':
			create_new = 1;
			config.id  = atoi(optarg);
			break;
		case 'a':
			attach_pid = atoi(optarg);
			if (!attach_pid)
				usage("-a: invalid PID");
			break;

		case 'r':
			config.id  = atoi(optarg);
			break;

		case 't':
			if (strcmp(optarg, "polling-periodic") == 0) {
				res_type = PERIODIC_POLLING;
			} else if (strcmp(optarg, "polling-sporadic") == 0) {
				res_type = SPORADIC_POLLING;
			}  else if (strcmp(optarg, "table-driven") == 0) {
				res_type = TABLE_DRIVEN;
			} else {
				usage("Unknown reservation type.");
			}
			break;

		case 'h':
			usage("");
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

	if (res_type != TABLE_DRIVEN) {
		config.polling_params.budget = ms2ns(budget_ms);
		config.polling_params.period = ms2ns(period_ms);
		config.polling_params.offset = ms2ns(offset_ms);
		config.polling_params.relative_deadline = ms2ns(deadline_ms);
		if (config.polling_params.budget > config.polling_params.period) {
			usage("The budget must not exceed the period.");
		}
	} else {
		config.table_driven_params.major_cycle_length = ms2ns(major_cycle_ms);
		argc -= optind;
		argv += optind;
		config.table_driven_params.intervals = parse_td_intervals(
			argc, argv, &config.table_driven_params.num_intervals,
			config.table_driven_params.major_cycle_length);
		if (!config.table_driven_params.num_intervals)
			usage("Table-driven reservations require at least one interval to be specified.");
	}

	if (create_new) {
		ret = reservation_create(res_type, &config);
		if (ret < 0) {
			fprintf(stderr, "failed to create reservation %u (%m)\n",
				config.id);
			exit(1);
		}
	}

	if (attach_pid)
		attach_task(attach_pid, &config);

	return 0;
}
