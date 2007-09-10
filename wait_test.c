#include <stdio.h>
#include <stdlib.h>
#include <signal.h>
#include <unistd.h>
#include <sys/time.h>
#include <sys/wait.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>

#include "litmus.h"

#define US_PER_MS 1000

#define NUMTASKS 4

int  prefix(void) 
{
	char field[1024];
	int prefix[1024];
	int i, sum = 0;

	for (i = 0; i < 1024; i++) {
		sum += field[i];
		prefix[i] = sum;
	}
	return sum;
}


void do_stuff(void)
{
	int i =0, j =0;

	for (; i < 50000; i++)
		j += prefix();
}

#define CALL(sc) do { ret = sc; if (ret == -1) {perror(" (!!) " #sc " failed: "); /*exit(1)*/;}} while (0);

unsigned int job_no;


void next(void)
{
	int ret;
	unsigned int actual;
	CALL(get_job_no(&actual));
	CALL(wait_for_job_release(++job_no));
	printf("Now executing job %u, waited for %u\n", actual, job_no);
}

void sync_jobs(void)
{
	int ret;
	unsigned int actual;
	CALL(get_job_no(&actual));
	job_no = actual;
}


int main(int argc, char** argv) 
{
	int ret;

	init_litmus();

	CALL(getpid());
	printf("my pid is %d\n", ret);


	CALL(get_job_no(&job_no));
	printf("my job_no is %u", job_no);

	next();
	next();
	next();
	
	/* now overrun a job for good */
	do_stuff();
	do_stuff();
	do_stuff();
	do_stuff();
	do_stuff();
	do_stuff();
	do_stuff();
	do_stuff();
	do_stuff();
	do_stuff();


	next();
	next();
	next();
	next();
	sync_jobs();
	next();
	next();
	next();


	return 0;	
}
