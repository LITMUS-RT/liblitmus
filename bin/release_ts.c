#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>

#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>

#include "common.h"
#include "litmus.h"
#include "internal.h"

#define OPTSTR "d:wf:Wq:"

#define LITMUS_STATS_FILE "/proc/litmus/stats"

void usage(char *error) {
	fprintf(stderr,
		"%s\n"
		"Usage: release_ts [OPTIONS]\n"
		"\n"
		"Options: -d  <delay in ms>  (default: 1000ms)\n"
		"         -q  <quantum in ms> (default: 1000ms)\n"
		"             (times release to occur on an integer multiple "
		              "of the quantum length)\n"
		"         -w  wait until all tasks are ready for release\n"
		"             (as determined by /proc/litmus/stats\n"
		"         -f  <#tasks> wait for #tasks (default: 0)\n"
		"         -W  just wait, don't actually release tasks\n"
		"\n",
		error);
	exit(1);
}

void wait_until_ready(int expected)
{
	int ready = 0, all = 0;
	int loops = 0;

	do {
		if (loops++ > 0)
			sleep(1);
		if (!read_litmus_stats(&ready, &all))
			perror("read_litmus_stats");
	} while (expected > ready || (!expected && ready < all));
}

int main(int argc, char** argv)
{
	int released;
	lt_t delay = ms2ns(1000);
	lt_t quantum = ms2ns(1000);
	lt_t when = 0;
	int wait = 0;
	int expected = 0;
	int exit_after_wait = 0;
	int opt;

	while ((opt = getopt(argc, argv, OPTSTR)) != -1) {
		switch (opt) {
		case 'd':
			delay = ms2ns(want_non_negative_double(optarg, "-d"));
			break;
		case 'q':
			quantum = ms2ns(want_non_negative_double(optarg, "-q"));
		case 'w':
			wait = 1;
			break;
		case 'W':
			wait = 1;
			exit_after_wait = 1;
			break;
		case 'f':
			wait = 1;
			expected = want_non_negative_int(optarg, "-f");
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

	if (wait)
		wait_until_ready(expected);

	if (exit_after_wait)
		exit(0);

	when = litmus_clock() + delay;
	when = ((when / quantum) + 1) * quantum;
	printf("Synchronous release at time %.2fms.\n",
		ns2ms((double) when));
	released = release_ts(&when);
	if (released < 0) {
		perror("release task system");
		exit(1);
	}

	printf("Released %d real-time tasks.\n", released);

	return 0;
}

