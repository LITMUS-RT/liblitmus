#include <stdio.h>
#include <stdlib.h>

#include <sys/types.h>
#include <sys/stat.h>
#include <sys/mman.h>
#include <fcntl.h>
#include <unistd.h>

#include "litmus.h"
#include "adaptive.h"
#include "sched_trace.h"


int walk_sched_trace(void* start, void* end, record_callback_t *cb)
{
	void* pos = start;
	trace_header_t* header;
	int ret;
	
	while (pos < end) {
		header = (trace_header_t*) pos;
		if (header->trace >= ST_MAX)
			return INVALID_HEADER;
		if (cb->handler[header->trace])
			ret = cb->handler[header->trace](header);
		if (ret)
			return ret;
		pos += header->size;
	}
	return 0;
}

int walk_sched_traces_ordered(void** start, void** end, unsigned int count,
			     record_callback_t *cb)
{
	void** pos;
	trace_header_t* header;
	int ret, i, adv;

	pos = malloc(sizeof(void*) * count);

	for (i = 0; i < count; i++)
		pos[i] = start[i];

	do {
		header = NULL;
		for (i = 0; i < count; i++)
			if (pos[i] < end[i] &&  
			    (!header || header->timestamp >
			     ((trace_header_t*) pos[i])->timestamp)) {
				header = (trace_header_t*) pos[i];
				adv = i;
			}
		if (header) {
			pos[i] += header->size; 
			if (header->trace >= ST_MAX)
				return INVALID_HEADER;
			if (cb->handler[header->trace])
				ret = cb->handler[header->trace](header);
		}
	} while (header != NULL && !ret);

	free(pos);
	return 0;
}

static int map_file(const char* filename, void **addr, size_t *size) 
{
	struct stat info;
	int error = 0;
	int fd;
	
	error = stat(filename, &info);
	if (!error) {
		*size = info.st_size;
		if (info.st_size > 0) {
			fd = open(filename, O_RDONLY);
			if (fd >= 0) {				
				*addr = mmap(NULL, *size, 
					     PROT_READ | PROT_WRITE, 
					     MAP_PRIVATE, fd, 0);
				if (*addr == MAP_FAILED)
					error = -1;
				close(fd);
			} else
				error = fd;			
		} else
			*addr = NULL;		
	}
	return error;
}

static int map_trace(const char *name, void **start, void **end, size_t *size)
{
	int ret;	

	ret = map_file(name, start, size);
	if (!ret)
		*end = *start + *size;
	return ret;
}

int walk_sched_trace_file(const char* name, int keep_mapped, 
			  record_callback_t *cb)
{
	int ret;	
	size_t size;
	void *start, *end;

	ret = map_trace(name, &start, &end, &size);
	if (!ret)
		ret = walk_sched_trace(start, end, cb);
	
	if (!keep_mapped)
		munmap(start, size);
	return ret;
}


int walk_sched_trace_files_ordered(const char** names, unsigned int count, 
				   int keep_mapped, record_callback_t *cb)
{
	void **start, **end;	
	size_t *size;
	int i;
	int ret;
	
	start = malloc(sizeof(void*) * count);
        end   = malloc(sizeof(void*) * count);
	size  = malloc(sizeof(size_t) * count);

	for (i = 0; i < count; i++)
		size[i] = 0;
	for (i = 0; i < count && !ret; i++)
		ret = map_trace(names[i], start + i, end + i, size + i);
	
	if (!ret)
		/* everything mapped, now walk it */
		ret = walk_sched_traces_ordered(start, end, count, cb);

	if (!keep_mapped)
		for (i = 0; i < count; i++)
			if (size[i])
				munmap(start[i], size[i]);
	
	free(start);
	free(end);
	free(size);
	return ret;
}
