#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>

#include "litmus.h"

#define OPTSTR "d:"
#define NS_PER_MS 1000000

void usage(char *error) {
	fprintf(stderr,
		"%s\n"
		"Usage: release_ts [-d <delay in ms>]\n",
		error);
	exit(1);
}


int main(int argc, char** argv)
{
	int released;
	lt_t delay = ms2lt(1000);
	int opt;
      
	while ((opt = getopt(argc, argv, OPTSTR)) != -1) {
		switch (opt) {
		case 'd':
			delay = ms2lt(atoi(optarg));
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

	released = release_ts(&delay);
	if (released < 0) {
		perror("release task system");
		exit(1);
	}
	
	printf("Released %d real-time tasks.\n", released);

	return 0;
}

