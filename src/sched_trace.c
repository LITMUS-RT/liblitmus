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
	*addr = mmap(NULL, *size, PROT_READ | PROT_WRITE, MAP_PRIVATE, fd, 0);
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

int walk_sched_trace_file(const char* name, int keep_mapped, 
			  record_callback_t *cb)
{
	int ret;	
	size_t size;
	void *start, *end;

	ret = map_file(name, &start, &size);
	if (!ret) {
		end = start + size;
		ret = walk_sched_trace(start, end, cb);
	}
	if (!keep_mapped)
		munmap(start, size);
	return ret;
}


