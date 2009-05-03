#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>

#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>

#include "litmus.h"

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

ssize_t read_file(const char* fname, void* buf, size_t maxlen)
{
	int fd;
	ssize_t n = 0;
	size_t got = 0;

	fd = open(fname, O_RDONLY);
	if (fd == -1)
		return -1;
	
	while (got < maxlen && (n = read(fd, buf + got, maxlen - got)) > 0)
		got += n;
	close(fd);
	if (n < 0)
		return -1;
	else
		return got;
}

void wait_until_ready(int expected)
{
	int ready = 0, all = 0;
	char buf[100];
	int loops = 0;
	ssize_t len;
	

	do {
		if (loops++ > 0)
			sleep(1);
		len = read_file(LITMUS_STATS_FILE, buf, sizeof(buf) - 1);
		if (len < 0) {
			fprintf(stderr,
				"(EE) Error while reading '%s': %m.\n"
				"(EE) Ignoring -w option.\n",
				LITMUS_STATS_FILE);
			break;
		} else {
			len = sscanf(buf,
				     "real-time tasks   = %d\n"
				     "ready for release = %d\n",
				     &all, &ready);
			if (len != 2) {
				fprintf(stderr, 
					"(EE) Could not parse '%s'.\n"
					"(EE) Ignoring -w option.\n",
					LITMUS_STATS_FILE);
				break;
			}
		}
	} while (expected > ready || ready < all);
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

