#include <stdio.h>
#include <stdlib.h>
#include <string.h>


#include "litmus.h"
#include "sched_trace.h"



int main(int argc, char** argv)
{
	record_callback_t cb;
	int ret;
	
	init_record_callback(&cb);
	
	ret = walk_sched_trace_files_ordered(argv + 1, argc - 1, 0, &cb);
	if (ret != 0)
		perror("walk failed");
	return 0;
}
