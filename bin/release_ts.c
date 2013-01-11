#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>

#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>

#include "litmus.h"
#include "internal.h"

#define OPTSTR "d:wf:"
#define NS_PER_MS 1000000

#define LITMUS_STATS_FILE "/proc/litmus/stats"

void usage(char *error) {
	fprintf(stderr,
		"%s\n"
		"Usage: release_ts [OPTIONS]\n"
		"\n"
		"Options: -d  <delay in ms>  (default: 1000ms)\n"
		"         -w  wait until all tasks are ready for release\n"
		"             (as determined by /proc/litmus/stats\n"
		"         -f  <#tasks> wait for #tasks (default: 0)\n"
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
	lt_t delay = ms2lt(1000);
	int wait = 0;
	int expected = 0;
	int opt;
      
	while ((opt = getopt(argc, argv, OPTSTR)) != -1) {
		switch (opt) {
		case 'd':
			delay = ms2lt(atoi(optarg));
			break;
		case 'w':
			wait = 1;
			break;
		case 'f':
			wait = 1;
			expected = atoi(optarg);
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

	released = release_ts(&delay);
	if (released < 0) {
		perror("release task system");
		exit(1);
	}
	
	printf("Released %d real-time tasks.\n", released);

	return 0;
}

