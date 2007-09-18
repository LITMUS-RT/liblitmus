#include <stdio.h>
#include <stdlib.h>
#include <signal.h>
#include <unistd.h>
#include <sys/time.h>
#include <sys/wait.h>

#include "litmus.h"

#define US_PER_MS 1000

int iotest(void *nil) {		
	int id = getpid();
	FILE* file;
	char str[255];
	unsigned long last = 0;
	struct timeval time;

	printf("I'am real time task %d  doing IO!\n", id);
	snprintf(str, sizeof(str), "rt-io-%d.txt", id);
	file = fopen(str, "w");
	if (!file) {
		perror("could not open file for output");
		exit(1);
	}
	while (1) {
		gettimeofday(&time, NULL);
		if (time.tv_usec - last > US_PER_MS) {
		  fprintf(file, "ran at %lus %lums\n", time.tv_sec, time.tv_usec / US_PER_MS);
		  last = time.tv_usec;
		}
		fflush(file);
	}
	return id;
}

#define NUMTASKS 4

int main(int argc, char** argv) 
{
	int rt_task[NUMTASKS];
	int i;
	int ret, pid;
	
	for (i = 0; i < NUMTASKS; i++) {		
		/*                        func   arg  cpu  wcet  period */
		rt_task[i] = create_rt_task(iotest, NULL,   0,   25,     100);
		if (rt_task[i] < 0) {
			perror("Could not create rt child process");
		}
	}

	sync();
	sync();

	printf(":: Starting real-time mode.\n");
	set_rt_mode(MODE_RT_RUN);

	printf(":: Sleeping...\n");
	sleep(120);	

	printf("Killing real-time tasks.\n");
	for (i = 0; i < NUMTASKS; i++) {
		printf(":: sending SIGKILL to %d\n", rt_task[i]);
		kill(rt_task[i], SIGKILL);			
	}
	for (i = 0; i < NUMTASKS; i++) {
		pid = wait(&ret);
		printf(":: %d exited with status %d\n", pid, ret);
	}
	printf(":: Leaving real-time mode.\n");
	set_rt_mode(MODE_NON_RT);
	return 0;	
}
