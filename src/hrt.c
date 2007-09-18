#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "edf-hsb.h"


void usage(char *name) 
{
	fprintf(stderr, 
		"EDF-HSB server setup utility\n"
		"Usage: %s hrt show    <#cpu>\n"
		"       %s hrt set     <#cpu> <wcet> <period>\n"
		"       %s be  create         <wcet> <period>\n", 
		name, name, name);
	exit(1);
}


int hrt(int argc, char** argv) 
{
	int wcet, period, cpu;

	if (argc == 2 && !strcmp(argv[0], "show")) {
		cpu = atoi(argv[1]);
		if (!get_hrt(cpu, &wcet, &period))
			printf("HRT/%d = (%d, %d)\n", cpu, wcet, period);
		else
			perror("cannot read HRT settings");
	} else if (argc == 4 && !strcmp(argv[0], "set")) {
		cpu    = atoi(argv[1]);
		wcet   = atoi(argv[2]);
		period = atoi(argv[3]);
		printf("Setting HRT/%d to (%d, %d)", cpu, wcet, period);
		if (!set_hrt(cpu, wcet, period))
			printf(" OK.\n");
		else {
			printf("\n");
			perror("cannot write HRT settings");
		}	
	} else
		return 1;

	return 0;
}

int be(int argc, char** argv)
{
	int wcet, period;
	if (argc == 3 && !strcmp(argv[0], "create")) {
		wcet    = atoi(argv[1]);
		period   = atoi(argv[2]);
		printf("Creating BE with (%d, %d)", wcet, period);
		if (!create_be(wcet, period))
			printf(" OK.\n");
		else {
			printf("\n");
			perror("cannot create BE server");
		}
		return 0;
	}
	else
		return 1;
}

int main(int argc, char** argv) 
{
	int ret = 1;
	if (argc > 1) {
		if (!strcmp(argv[1], "hrt"))
			ret = hrt(argc - 2, argv + 2);
		else if (!strcmp(argv[1], "be"))
			ret = be(argc - 2, argv + 2);
	}
	if (ret)
		usage(argv[0]);
	return ret;
}
