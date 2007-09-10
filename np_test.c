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


int main(int argc, char** argv) 
{
	int ret;

	init_litmus();

	enter_np();

	do_stuff();
	
	exit_np();


	return 0;	
}
