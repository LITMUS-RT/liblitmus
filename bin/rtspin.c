#include <sys/time.h>

#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <limits.h>
#include <time.h>
#include <string.h>
#include <assert.h>
#include <inttypes.h>
#include <sys/mman.h>
#include <errno.h>

#include "litmus.h"
#include "common.h"

const char *usage_msg =
	"Usage: (1) rtspin OPTIONS WCET PERIOD DURATION\n"
	"       (2) rtspin -S [INPUT] WCET PERIOD DURATION\n"
	"       (3) rtspin OPTIONS -C FILE:COLUMN WCET PERIOD [DURATION]\n"
	"       (4) rtspin -l\n"
	"       (5) rtspin -B -m FOOTPRINT\n"
	"\n"
	"Modes: (1) run as periodic task with given WCET and PERIOD\n"
	"       (2) run as sporadic task with given WCET and PERIOD,\n"
	"           using INPUT as a file from which events are received\n"
	"           by means of blocking reads (default: read from STDIN)\n"
	"       (3) as (1) or (2), but load per-job execution times from\n"
	"           the given column of a CSV file\n"
	"       (4) Run calibration loop (how accurately are target\n"
	"           runtimes met?)\n"
	"       (5) Run background, non-real-time cache-thrashing loop.\n"
	"\n"
	"Required arguments:\n"
	"    WCET, PERIOD      reservation parameters (in ms)\n"
	"    DURATION          terminate the task after DURATION seconds\n"
	"\n"
	"Options:\n"
	"    -B                run non-real-time background loop\n"
	"    -c be|srt|hrt     task class (best-effort, soft real-time, hard real-time)\n"
	"    -d DEADLINE       relative deadline, equal to the period by default (in ms)\n"
	"    -e                turn on budget enforcement (off by default)\n"
	"    -h                show this help message\n"
	"    -i                report interrupts (implies -v)\n"
	"    -l                run calibration loop and report error\n"
	"    -m FOOTPRINT      specify number of data pages to access\n"
	"    -o OFFSET         offset (also known as phase), zero by default (in ms)\n"
	"    -p CPU            partition or cluster to assign this task to\n"
	"    -q PRIORITY       priority to use (ignored by EDF plugins, highest=1, lowest=511)\n"
	"    -r VCPU           virtual CPU or reservation to attach to (irrelevant to most plugins)\n"
	"    -R                create sporadic reservation for task (with VCPU=PID)\n"
	"    -s SCALE          fraction of WCET to spin for (1.0 means 100%, default 0.95)\n"
	"    -u SLACK          randomly under-run WCET by up to SLACK milliseconds\n"
	"    -U SLACK-FRACTION randomly under-run WCET by up to (WCET * SLACK-FRACTION) milliseconds \n"
	"    -v                verbose (print per-job statistics)\n"
	"    -w                wait for synchronous release\n"
	"\n"
	"    -C FILE[:COLUMN]  load per-job execution times from CSV file;\n"
	"                      if COLUMN is given, it specifies the column to read\n"
	"                      per-job execution times from (default: 1)\n"
	"    -A FILE[:COLUMN]  load sporadic inter-arrival times from CSV file (implies -T);\n"
	"                      if COLUMN is given, it specifies the column to read\n"
	"                      inter-arrival times from (default: 1)\n"
	"\n"
	"    -S[FILE]          read from FILE to trigger sporadic job releases\n"
	"                      default w/o -S: periodic job releases\n"
	"                      default if FILE is omitted: read from STDIN\n"
	"    -O[FILE]          write to FILE when job completes (this is useful with -S\n"
	"                      to create precedence constraints/event chains)\n"
	"                      default w/o -O: no output\n"
	"                      default if FILE is omitted: write to STDOUT\n"
	"\n"
	"    -T                use clock_nanosleep() instead of sleep_next_period()\n"
	"    -D MAX-DELTA      set maximum inter-arrival delay to MAX-DELTA [default: period]\n"
	"    -E MIN-DELTA      set minimum inter-arrival delay to MIN-DELTA [default: period]\n"
	"\n"
	"    -X PROTOCOL       access a shared resource protected by a locking protocol\n"
	"    -L CS-LENGTH      simulate a critical section length of CS-LENGTH milliseconds\n"
	"    -Q RESOURCE-ID    access the resource identified by RESOURCE-ID\n"
	"\n"
	"Units:\n"
	"    WCET and PERIOD are expected in milliseconds.\n"
	"    SLACK is expected in milliseconds.\n"
	"    DURATION is expected in seconds.\n"
	"    CS-LENGTH is expected in milliseconds.\n"
	"    FOOTPRINT is expected in number of pages\n";


static void usage(char *error) {
	if (error)
		fprintf(stderr, "Error: %s\n\n", error);
	else {
		fprintf(stderr, "rtspin: simulate a periodic or sporadic "
		                "CPU-bound real-time task\n\n");
	}
	fprintf(stderr, "%s", usage_msg);
	exit(error ? EXIT_FAILURE : EXIT_SUCCESS);
}

#define NUMS 4096
static int num[NUMS];
static char* progname;

static int nr_of_pages = 0;
static int page_size;
static void *base = NULL;

static int loop_once(void)
{
	int i, j = 0;
	/* touch some numbers and do some math */
	for (i = 0; i < NUMS; i++) {
		j += num[i]++;
		if (j > num[i])
			num[i] = (j / 2) + 1;
	}
	return j;
}

static int loop_once_with_mem(void)
{
	int i, j = 0;
	int rand;
	int *num;

	/* choose a random page */
	if (nr_of_pages > 1)
		rand = lrand48() % (nr_of_pages - 1);
	else
		rand = 0;

	/* touch the randomly selected page */
	num = base + (rand * page_size);
	for (i = 0; i < page_size / sizeof(int); i++) {
		j += num[i]++;
		if (j > num[i])
			num[i] = (j / 2) + 1;
	}

	return j;
}

static int loop_for(double exec_time, double emergency_exit)
{
	double last_loop = 0, loop_start;
	int tmp = 0;

	double start = cputime();
	double now = cputime();

	while (now + last_loop < start + exec_time) {
		loop_start = now;
		if (nr_of_pages)
			tmp += loop_once_with_mem();
		else
			tmp += loop_once();
		now = cputime();
		last_loop = now - loop_start;
		if (emergency_exit && wctime() > emergency_exit) {
			/* Oops --- this should only be possible if the
			 * execution time tracking is broken in the LITMUS^RT
			 * kernel or the user specified infeasible parameters.
			 */
			fprintf(stderr, "!!! rtspin/%d emergency exit!\n",
			        getpid());
			fprintf(stderr, "Reached experiment timeout while "
			        "spinning.\n");
			break;
		}
	}

	return tmp;
}


static void debug_delay_loop(void)
{
	double start, end, delay;

	while (1) {
		for (delay = 0.5; delay > 0.01; delay -= 0.01) {
			start = wctime();
			loop_for(delay, 0);
			end = wctime();
			printf("%6.4fs: looped for %10.8fs, delta=%11.8fs, error=%7.4f%%\n",
			       delay,
			       end - start,
			       end - start - delay,
			       100 * (end - start - delay) / delay);
		}
	}
}

static char input_buf[4096] = "<no input>";

static int wait_for_input(int event_fd)
{
	/* We do a blocking read, accepting up to 4KiB of data.
	 * For simplicity, for now, if there's more than 4KiB of data,
	 * we treat this as multiple jobs. Note that this means that
	 * tardiness can result in coalesced jobs. Ideally, there should
	 * be some sort of configurable job boundary marker, but that's
	 * not supported in this basic version yet. Patches welcome.
	 */
	size_t consumed;

	consumed = read(event_fd, input_buf, sizeof(input_buf) - 1);

	if (consumed == 0)
		fprintf(stderr, "reached end-of-file on input event stream\n");
	if (consumed < 0)
		fprintf(stderr, "error reading input event stream (%m)\n");

	if (consumed > 0) {
		/* zero-terminate string buffer */
		input_buf[consumed] = '\0';
		/* check if we can remove a trailing newline */
		if (consumed > 1 && input_buf[consumed - 1] == '\n') {
			input_buf[consumed - 1] = '\0';
		}
	}

	return consumed > 0;
}

static int generate_output(int output_fd)
{
	char buf[4096];
	size_t len, written;
	unsigned int job_no;

	get_job_no(&job_no);
	len = snprintf(buf, 4095, "(rtspin/%d:%u completed: %s @ %" PRIu64 "ns)\n",
			getpid(), job_no, input_buf, (uint64_t) litmus_clock());

	written = write(output_fd, buf, len);

	return written == len;
}

static void job(double exec_time, double program_end, int lock_od, double cs_length)
{
	double chunk1, chunk2;

	if (lock_od >= 0) {
		/* simulate critical section somewhere in the middle */
		chunk1 = drand48() * (exec_time - cs_length);
		chunk2 = exec_time - cs_length - chunk1;

		/* non-critical section */
		loop_for(chunk1, program_end + 1);

		/* critical section */
		litmus_lock(lock_od);
		loop_for(cs_length, program_end + 1);
		litmus_unlock(lock_od);

		/* non-critical section */
		loop_for(chunk2, program_end + 2);
	} else {
		loop_for(exec_time, program_end + 1);
	}
}

static lt_t choose_inter_arrival_time_ns(
	double* arrival_times, int num_arrivals, int cur_job,
	double range_min, double range_max)
{
	double iat_ms;

	if (arrival_times)
		iat_ms = arrival_times[cur_job % num_arrivals];
	else
		iat_ms = range_min + drand48() * (range_max - range_min);

	return ms2ns(iat_ms);
}

#define OPTSTR "p:c:wlveo:s:m:q:r:X:L:Q:iRu:U:Bhd:C:S::O::TD:E:A:"

int main(int argc, char** argv)
{
	int ret;
	lt_t wcet;
	lt_t period, deadline;
	lt_t phase;
	lt_t inter_arrival_time;
	double inter_arrival_min_ms = 0, inter_arrival_max_ms = 0;
	double wcet_ms, period_ms, underrun_ms = 0;
	double underrun_frac = 0;
	double offset_ms = 0, deadline_ms = 0;
	unsigned int priority = LITMUS_NO_PRIORITY;
	int migrate = 0;
	int cluster = 0;
	int reservation = -1;
	int create_reservation = 0;
	int opt;
	int wait = 0;
	int test_loop = 0;
	int background_loop = 0;

	int cost_column = 1;
	const char *cost_csv_file = NULL;
	int num_jobs = 0;
	double *exec_times = NULL;

	int arrival_column = 1;
	const char *arrival_csv_file = NULL;
	int num_arrival_times = 0;
	double *arrival_times = NULL;

	int want_enforcement = 0;
	double duration = 0, start = 0;
	double scale = 0.95;
	task_class_t class = RT_CLASS_HARD;
	int cur_job = 0;
	struct rt_task param;

	char *after_colon;

	int rss = 0;
	int idx;

	int sporadic = 0;  /* trigger jobs sporadically? */
	int event_fd = -1; /* file descriptor for sporadic events */
	int want_output = 0; /* create output at end of job? */
	int output_fd = -1;  /* file descriptor for output */

	int linux_sleep = 0; /* use Linux API for periodic activations? */
	lt_t next_release;

	int verbose = 0;
	unsigned int job_no;
	struct control_page* cp;
	int report_interrupts = 0;
	uint64_t last_irq_count = 0;

	/* locking */
	int lock_od = -1;
	int resource_id = 0;
	const char *lock_namespace = "./rtspin-locks";
	int protocol = -1;
	double cs_length = 1; /* millisecond */

	progname = argv[0];

	while ((opt = getopt(argc, argv, OPTSTR)) != -1) {
		switch (opt) {
		case 'w':
			wait = 1;
			break;
		case 'p':
			cluster = want_non_negative_int(optarg, "-p");
			migrate = 1;
			break;
		case 'r':
			reservation = want_non_negative_int(optarg, "-r");
			break;
		case 'R':
			create_reservation = 1;
			reservation = getpid();
			break;
		case 'q':
			priority = want_non_negative_int(optarg, "-q");
			if (!litmus_is_valid_fixed_prio(priority))
				usage("Invalid priority.");
			break;
		case 'c':
			class = str2class(optarg);
			if (class == -1)
				usage("Unknown task class.");
			break;
		case 'e':
			want_enforcement = 1;
			break;
		case 'l':
			test_loop = 1;
			break;
		case 'B':
			background_loop = 1;
			break;
		case 'C':
			after_colon = strsplit(':', optarg);
			cost_csv_file = optarg;
			if (after_colon) {
				cost_column =
					want_non_negative_int(after_colon, "-C");
			}
			break;
		case 'A':
			after_colon = strsplit(':', optarg);
			arrival_csv_file = optarg;
			if (after_colon) {
				arrival_column =
					want_non_negative_int(after_colon, "-A");
			}
			linux_sleep = 1;
			break;
		case 'S':
			sporadic = 1;
			if (!optarg || strcmp(optarg, "-") == 0)
				event_fd = STDIN_FILENO;
			else
				event_fd = open(optarg, O_RDONLY);
			if (event_fd == -1) {
				fprintf(stderr, "Could not open file '%s' "
					"(%m)\n", optarg);
				usage("-S requires a valid file path or '-' "
				      "for STDIN.");
			}
			break;

		case 'O':
			want_output = 1;
			if (!optarg || strcmp(optarg, "-") == 0)
				output_fd = STDOUT_FILENO;
			else
				output_fd = open(optarg, O_WRONLY | O_APPEND);
			if (output_fd == -1) {
				fprintf(stderr, "Could not open file '%s' "
					"(%m)\n", optarg);
				usage("-O requires a valid file path or '-' "
				      "for STDOUT.");
			}
			break;

		case 'T':
			linux_sleep = 1;
			break;
		case 'D':
			linux_sleep = 1;
			inter_arrival_max_ms =
				want_non_negative_double(optarg, "-D");
			break;
		case 'E':
			linux_sleep = 1;
			inter_arrival_min_ms =
				want_non_negative_double(optarg, "-E");
			break;
		case 'm':
			nr_of_pages = want_non_negative_int(optarg, "-m");
			break;
		case 's':
			scale = want_non_negative_double(optarg, "-s");
			break;
		case 'o':
			offset_ms = want_non_negative_double(optarg, "-o");
			break;
		case 'd':
			deadline_ms = want_non_negative_double(optarg, "-d");
			break;
		case 'u':
			underrun_ms = want_positive_double(optarg, "-u");
			break;
		case 'U':
			underrun_frac = want_positive_double(optarg, "-U");
			if (underrun_frac > 1)
				usage("-U: argument must be in the range (0, 1]");
			break;
		case 'X':
			protocol = lock_protocol_for_name(optarg);
			if (protocol < 0)
				usage("Unknown locking protocol specified.");
			break;
		case 'L':
			cs_length = want_positive_double(optarg, "-L");
			break;
		case 'Q':
			resource_id = want_non_negative_int(optarg, "-Q");

			break;
		case 'v':
			verbose = 1;
			break;
		case 'h':
			usage(NULL);
			break;
		case 'i':
			verbose = 1;
			report_interrupts = 1;
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

	if (nr_of_pages) {
		page_size = getpagesize();
		rss = page_size * nr_of_pages;
		base = mmap(NULL, rss, PROT_WRITE, MAP_ANONYMOUS | MAP_PRIVATE, -1, 0);
		if(base == MAP_FAILED) {
			fprintf(stderr,"mmap failed: %s\n",strerror(errno));
			exit(EXIT_FAILURE);
		}

		/* pin frames to prevent swapping */
		ret = mlock(base,rss);
		if (ret) {
			fprintf(stderr,"mlock failed: %s\n",strerror(errno));
		}

		/* touch every allocated page */
		for(idx = 0; idx < nr_of_pages; idx++)
				memset(base + (idx * page_size), 1, page_size);
	}

	srand(getpid());

	if (test_loop) {
		debug_delay_loop();
		return 0;
	}

	if (background_loop) {
		while (1) {
			if (nr_of_pages)
				loop_once_with_mem();
			else
				loop_once();
		}
		return 0;
	}

	if (argc - optind < 3 || (argc - optind < 2 && !cost_csv_file))
		usage("Arguments missing.");

	wcet_ms   = want_positive_double(argv[optind + 0], "WCET");
	period_ms = want_positive_double(argv[optind + 1], "PERIOD");

	wcet   = ms2ns(wcet_ms);
	period = ms2ns(period_ms);
	phase  = ms2ns(offset_ms);
	deadline = ms2ns(deadline_ms);
	if (wcet <= 0)
		usage("The worst-case execution time must be a "
				"positive number.");
	if (offset_ms < 0)
		usage("The synchronous release delay must be a "
				"non-negative number.");

	if (period <= 0)
		usage("The period must be a positive number.");
	if (!cost_csv_file && wcet > period) {
		usage("The worst-case execution time must not "
				"exceed the period.");
	}

	if (cost_csv_file)
		exec_times = csv_read_column(cost_csv_file, cost_column,
					&num_jobs);

	if (arrival_csv_file)
		arrival_times = csv_read_column(arrival_csv_file,
					arrival_column, &num_arrival_times);


	if (argc - optind < 3 && cost_csv_file)
		/* If duration is not given explicitly,
		 * take duration from file. */
		duration = num_jobs * period_ms * 0.001;
	else
		duration = want_positive_double(argv[optind + 2], "DURATION");

	if (underrun_frac) {
		underrun_ms = underrun_frac * wcet_ms;
	}

	if (migrate) {
		ret = be_migrate_to_domain(cluster);
		if (ret < 0)
			bail_out("could not migrate to target partition or cluster.");
	}


	init_rt_task_param(&param);
	param.exec_cost = wcet;
	param.period = period;
	param.phase  = phase;
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
		config.polling_params.offset = phase;
		config.polling_params.relative_deadline = deadline;
		ret = reservation_create(SPORADIC_POLLING, &config);
		if (ret < 0)
			bail_out("failed to create reservation");
	}

	srand48(time(NULL));


	init_litmus();

	start = wctime();
	ret = task_mode(LITMUS_RT_TASK);
	if (ret != 0)
		bail_out("could not become RT task");

	cp = get_ctrl_page();

	if (protocol >= 0) {
		/* open reference to semaphore */
		lock_od = litmus_open_lock(protocol, resource_id, lock_namespace, &cluster);
		if (lock_od < 0) {
			perror("litmus_open_lock");
			usage("Could not open lock.");
		}
	}


	if (wait) {
		ret = wait_for_ts_release();
		if (ret != 0)
			bail_out("wait_for_ts_release()");
		start = wctime();
	}


	next_release = cp ? cp->release : litmus_clock();

	/* default: periodic releases */
	if (!inter_arrival_min_ms)
		inter_arrival_min_ms = period_ms;
	if (!inter_arrival_max_ms)
		inter_arrival_max_ms = period_ms;

	if (inter_arrival_min_ms > inter_arrival_max_ms)
		inter_arrival_max_ms = inter_arrival_min_ms;
	inter_arrival_time = period;

	/* main job loop */
	cur_job = 0;
	while (1) {
		double acet; /* actual execution time */

		if (sporadic) {
			/* sporadic job activations, sleep until
			 * we receive an "event" (= any data) from
			 * our input event channel */
			if (!wait_for_input(event_fd))
				/* error out of something goes wrong */
				break;
		}

		/* first, check if we have reached the end of the run */
		if (wctime() > start + duration)
			break;

		if (verbose) {
			get_job_no(&job_no);
			fprintf(stderr, "rtspin/%d:%u @ %.4fms\n", gettid(),
				job_no, (wctime() - start) * 1000);
			if (cp) {
				double deadline, current, release;
				lt_t now = litmus_clock();
				deadline = ns2s((double) cp->deadline);
				current  = ns2s((double) now);
				release  = ns2s((double) cp->release);
				fprintf(stderr,
				        "\trelease:  %" PRIu64 "ns (=%.2fs)\n",
				        (uint64_t) cp->release, release);
				fprintf(stderr,
				        "\tdeadline: %" PRIu64 "ns (=%.2fs)\n",
				        (uint64_t) cp->deadline, deadline);
				fprintf(stderr,
				        "\tcur time: %" PRIu64 "ns (=%.2fs)\n",
				        (uint64_t) now, current);
				fprintf(stderr,
				        "\ttime until deadline: %.2fms\n",
				        (deadline - current) * 1000);
			}
			if (report_interrupts && cp) {
				uint64_t irq = cp->irq_count;

				fprintf(stderr,
				        "\ttotal interrupts: %" PRIu64
				        "; delta: %" PRIu64 "\n",
				       irq, irq - last_irq_count);
				last_irq_count = irq;
			}
		}

		/* figure out for how long this job should use the CPU */

		if (cost_csv_file) {
			/* read from provided CSV file and convert to seconds */
			acet = exec_times[cur_job % num_jobs] * 0.001;
		} else {
			/* randomize and convert to seconds */
			acet = (wcet_ms - drand48() * underrun_ms) * 0.001;
			if (acet < 0)
				acet = 0;
		}
		/* scale exec time */
		acet *= scale;

		if (verbose)
			fprintf(stderr,
				"\ttarget exec. time: %6.2fms (%.2f%% of WCET)\n",
				acet * 1000,
				(acet * 1000 / wcet_ms) * 100);

		/* burn cycles */
		job(acet, start + duration, lock_od, cs_length * 0.001);

		if (want_output) {
			/* generate some output at end of job */
			generate_output(output_fd);
		}

		/* wait for periodic job activation (unless sporadic) */
		if (!sporadic) {
			/* periodic job activations */
			if (linux_sleep) {
				/* Use vanilla Linux API. This looks to the
				 * active LITMUS^RT plugin like a
				 * self-suspension. */

				inter_arrival_time =
					choose_inter_arrival_time_ns(
				        	arrival_times,
				                num_arrival_times,
				                cur_job,
				                inter_arrival_min_ms,
				                inter_arrival_max_ms);

				next_release += inter_arrival_time;

				if (verbose)
					fprintf(stderr,
				                "\tclock_nanosleep() until %"
					        PRIu64 "ns (=%.2fs), "
					        "delta %" PRIu64 "ns (=%.2fms)\n",
				                (uint64_t) next_release,
				                ns2s((double) next_release),
				                (uint64_t) inter_arrival_time,
				                ns2ms((double) inter_arrival_time));

				lt_sleep_until(next_release);

			} else {
				/* Use LITMUS^RT API: some plugins optimize
				 * this by not actually suspending the task. */
				if (verbose && cp)
					fprintf(stderr,
				                "\tsleep_next_period() until %"
					        PRIu64 "ns (=%.2fs)\n",
					        (uint64_t) (cp->release + period),
					        ns2s((double) (cp->release + period)));
				sleep_next_period();
			}
		}
		cur_job++;
	}

	ret = task_mode(BACKGROUND_TASK);
	if (ret != 0)
		bail_out("could not become regular task (huh?)");

	if (cost_csv_file)
		free(exec_times);

	if (base != MAP_FAILED)
		munlock(base, rss);

	return 0;
}
