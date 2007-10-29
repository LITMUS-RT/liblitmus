#include <stdio.h>
#include <stdlib.h>
#include <string.h>


#include "litmus.h"
#include "adaptive.h"
#include "sched_trace.h"

int show_sl_chg(trace_header_t* hdr)
{
	service_level_change_record_t *rec;

	rec = (service_level_change_record_t*) hdr;
	printf("SL CHANGE : PID=%d PERIOD=%lu\n", rec->task.pid, 
	       rec->new_level.period);
	return 0;
}

int show_weight_error(trace_header_t* hdr)
{
	weight_error_record_t *rec;

	rec = (weight_error_record_t*) hdr;
	printf("WEIGHT ERR: PID=%d EST=%5.4f ACT=%5.4f\n", rec->task, 
	       fp2f(rec->estimate), fp2f(rec->actual));
	return 0;
}


int main(int argc, char** argv)
{
	record_callback_t cb;
	int ret;
	
	init_record_callback(&cb);
	set_callback(ST_SERVICE_LEVEL_CHANGE, show_sl_chg, &cb);
	set_callback(ST_WEIGHT_ERROR, show_weight_error, &cb);	
	
	ret = walk_sched_trace_files_ordered(argv + 1, argc - 1, 0, &cb);
	if (ret != 0)
		perror("walk failed");
	return 0;
}
