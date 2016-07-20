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
	"       (3) rtspin OPTIONS -F FILE [-C COLUMN] WCET PERIOD [DURATION]\n"
	"       (4) rtspin -l\n"
	"       (5) rtspin -B -m FOOTPRINT\n"
	"\n"
	"Modes: (1) run as periodic task with given WCET and PERIOD\n"
	"       (2) run as sporadic task with given WCET and PERIOD,\n"
	"           using INPUT as a file from which events are received\n"
	"           by means of blocking reads (default: read from STDIN)\n"
	"       (3) as (1) or (2), but load per-job execution times from\n"
	"           a CSV file\n"
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
	"    -s SCALE          fraction of WCET to spin for (1.0 means 100%)\n"
	"    -u SLACK          randomly under-run WCET by up to SLACK milliseconds\n"
	"    -v                verbose (print per-job statistics)\n"
	"    -w                wait for synchronous release\n"
	"\n"
	"    -F FILE           load per-job execution times from CSV file\n"
	"    -C COLUMN         specify column to read per-job execution times from (default: 1)\n"
	"\n"
	"    -S [FILE]         read from FILE to trigger sporadic job releases\n"
	"                      default w/o -S: periodic job releases\n"
	"                      default if FILE is omitted: read from STDIN\n"
	"\n"
	"    -T                use clock_nanosleep() instead of sleep_next_period()\n"
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
		fprintf(stderr, "rtspin: simulate a periodic CPU-bound "
		                "real-time task\n\n");
	}
	fprintf(stderr, "%s", usage_msg);
	exit(error ? EXIT_FAILURE : EXIT_SUCCESS);
}

/*
 * returns the character that made processing stop, newline or EOF
 */
static int skip_to_next_line(FILE *fstream)
{
	int ch;	for (ch = fgetc(fstream); ch != EOF && ch != '\n'; ch = fgetc(fstream));
	return ch;
}

static void skip_comments(FILE *fstream)
{
	int ch;
	for (ch = fgetc(fstream); ch == '#'; ch = fgetc(fstream))
		skip_to_next_line(fstream);
	ungetc(ch, fstream);
}

static void get_exec_times(const char *file, const int column,
			   int *num_jobs,    double **exec_times)
{
	FILE *fstream;
	int  cur_job, cur_col, ch;
	*num_jobs = 0;

	fstream = fopen(file, "r");
	if (!fstream)
		bail_out("could not open execution time file");

	/* figure out the number of jobs */
	do {
		skip_comments(fstream);
		ch = skip_to_next_line(fstream);
		if (ch != EOF)
			++(*num_jobs);
	} while (ch != EOF);

	if (-1 == fseek(fstream, 0L, SEEK_SET))
		bail_out("rewinding file failed");

	/* allocate space for exec times */
	*exec_times = calloc(*num_jobs, sizeof(*exec_times));
	if (!*exec_times)
		bail_out("couldn't allocate memory");

	for (cur_job = 0; cur_job < *num_jobs && !feof(fstream); ++cur_job) {

		skip_comments(fstream);

		for (cur_col = 1; cur_col < column; ++cur_col) {
			/* discard input until we get to the column we want */
			int unused __attribute__ ((unused)) = fscanf(fstream, "%*s,");
		}

		/* get the desired exec. time */
		if (1 != fscanf(fstream, "%lf", (*exec_times)+cur_job)) {
			fprintf(stderr, "invalid execution time near line %d\n",
					cur_job);
			exit(EXIT_FAILURE);
		}

		skip_to_next_line(fstream);
	}

	assert(cur_job == *num_jobs);
	fclose(fstream);
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
	for (i = 0; i < NUMS; i++)
		j += num[i]++;
	return j;
}

static int loop_once_with_mem(void)
{
	int i;
	int rand;
	for(i=0; i<NUMS; i++) {
		rand=lrand48() % (nr_of_pages - 1);
		memset(base + (rand * page_size),rand,1024);
	}
	return 0;
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

static int wait_for_input(int event_fd)
{
	/* We do a blocking read, accepting up to 4KiB of data.
	 * For simplicity, for now, if there's more than 4KiB of data,
	 * we treat this as multiple jobs. Note that this means that
	 * tardiness can result in coalesced jobs. Ideally, there should
	 * be some sort of configurable job boundary marker, but that's
	 * not supported in this basic version yet. Patches welcome.
	 */
	char buf[4096];
	size_t consumed;

	consumed = read(event_fd, buf, sizeof(buf));

	if (consumed == 0)
		fprintf(stderr, "reached end-of-file on input event stream\n");
	if (consumed < 0)
		fprintf(stderr, "error reading input event stream (%m)\n");

	return consumed > 0;
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

#define OPTSTR "p:c:wlveo:F:s:m:q:r:X:L:Q:iRu:Bhd:C:S::T"
int main(int argc, char** argv)
{
	int ret;
	lt_t wcet;
	lt_t period, deadline;
	lt_t phase;
	double wcet_ms, period_ms, underrun_ms = 0;
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
	int column = 1;
	const char *file = NULL;
	int want_enforcement = 0;
	double duration = 0, start = 0;
	double *exec_times = NULL;
	double scale = 1.0;
	task_class_t class = RT_CLASS_HARD;
	int cur_job = 0, num_jobs = 0;
	struct rt_task param;

	int rss = 0;
	int idx;

	int sporadic = 0;  /* trigger jobs sporadically? */
	int event_fd = -1; /* file descriptor for sporadic events */

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
			cluster = atoi(optarg);
			migrate = 1;
			break;
		case 'r':
			reservation = atoi(optarg);
			break;
		case 'R':
			create_reservation = 1;
			reservation = getpid();
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
			want_enforcement = 1;
			break;
		case 'l':
			test_loop = 1;
			break;
		case 'B':
			background_loop = 1;
			break;
		case 'C':
			column = atoi(optarg);
			break;
		case 'F':
			file = optarg;
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
		case 'T':
			linux_sleep = 1;
			break;
		case 'm':
			nr_of_pages = atoi(optarg);
			break;
		case 's':
			scale = atof(optarg);
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
		case 'u':
			underrun_ms = atof(optarg);
			if (underrun_ms <= 0)
				usage("-u: positive argument needed.");
			break;
		case 'X':
			protocol = lock_protocol_for_name(optarg);
			if (protocol < 0)
				usage("Unknown locking protocol specified.");
			break;
		case 'L':
			cs_length = atof(optarg);
			if (cs_length <= 0)
				usage("Invalid critical section length.");
			break;
		case 'Q':
			resource_id = atoi(optarg);
			if (resource_id <= 0 && strcmp(optarg, "0"))
				usage("Invalid resource ID.");
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

	if (argc - optind < 3 || (argc - optind < 2 && !file))
		usage("Arguments missing.");

	wcet_ms   = atof(argv[optind + 0]);
	period_ms = atof(argv[optind + 1]);

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
	if (!file && wcet > period) {
		usage("The worst-case execution time must not "
				"exceed the period.");
	}

	if (file)
		get_exec_times(file, column, &num_jobs, &exec_times);

	if (argc - optind < 3 && file)
		/* If duration is not given explicitly,
		 * take duration from file. */
		duration = num_jobs * period_ms * 0.001;
	else
		duration = atof(argv[optind + 2]);

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

	if (cp)
		next_release = cp->release + period;
	else
		next_release = litmus_clock() + period;

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
			printf("rtspin/%d:%u @ %.4fms\n", gettid(),
				job_no, (wctime() - start) * 1000);
			if (cp) {
				double deadline, current, release;
				lt_t now = litmus_clock();
				deadline = ns2s((double) cp->deadline);
				current  = ns2s((double) now);
				release  = ns2s((double) cp->release);
				printf("\trelease:  %" PRIu64 "ns (=%.2fs)\n",
				       (uint64_t) cp->release, release);
				printf("\tdeadline: %" PRIu64 "ns (=%.2fs)\n",
				       (uint64_t) cp->deadline, deadline);
				printf("\tcur time: %" PRIu64 "ns (=%.2fs)\n",
				       (uint64_t) now, current);
				printf("\ttime until deadline: %.2fms\n",
				       (deadline - current) * 1000);
			}
			if (report_interrupts && cp) {
				uint64_t irq = cp->irq_count;

				printf("\ttotal interrupts: %" PRIu64
				       "; delta: %" PRIu64 "\n",
				       irq, irq - last_irq_count);
				last_irq_count = irq;
			}
		}

		/* figure out for how long this job should use the CPU */
		cur_job++;

		if (file) {
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
			printf("\ttarget exec. time: %6.2fms (%.2f%% of WCET)\n",
				acet * 1000,
				(acet * 1000 / wcet_ms) * 100);

		/* burn cycles */
		job(acet, start + duration, lock_od, cs_length * 0.001);

		/* wait for periodic job activation (unless sporadic) */
		if (!sporadic) {
			/* periodic job activations */
			if (linux_sleep) {
				/* Use vanilla Linux API. This looks to the
				 * active LITMUS^RT plugin like a
				 * self-suspension. */
				if (verbose)
					printf("\tclock_nanosleep() until %"
					       PRIu64 "ns (=%.2fs)\n",
				               (uint64_t) next_release,
				               ns2s((double) next_release));
				lt_sleep_until(next_release);
				next_release += period;
			} else {
				/* Use LITMUS^RT API: some plugins optimize
				 * this by not actually suspending the task. */
				if (verbose && cp)
					printf("\tsleep_next_period() until %"
					       PRIu64 "ns (=%.2fs)\n",
					       (uint64_t) (cp->release + period),
					       ns2s((double) (cp->release + period)));
				sleep_next_period();
			}
		}
	}

	ret = task_mode(BACKGROUND_TASK);
	if (ret != 0)
		bail_out("could not become regular task (huh?)");

	if (file)
		free(exec_times);

	if (base != MAP_FAILED)
		munlock(base, rss);

	return 0;
}
