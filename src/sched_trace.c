#include <stdio.h>
#include <stdlib.h>

#include <sys/types.h>
#include <sys/stat.h>
#include <sys/mman.h>
#include <fcntl.h>
#include <unistd.h>

#include "litmus.h"
#include "sched_trace.h"

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

static const char* event_names[] = {
	"INVALID",
        "NAME",
	"PARAM",
        "RELEASE",
	"ASSIGNED",
	"SWITCH_TO",
	"SWITCH_FROM",
	"COMPLETION",
	"BLOCK",
	"RESUME",
	"INVALID"
};

#define ST_INVALID (ST_RESUME + 1)

const char* event2name(unsigned int id)
{
	if (id >= ST_INVALID)
		id = ST_INVALID;
	return event_names[id];
}

void print_header(struct st_trace_header* hdr)
{
	printf("%-14s on CPU %u for %5u/%5u",
	       event2name(hdr->type),
	       hdr->cpu, hdr->pid, hdr->job);
}

void print_all(struct st_event_record *rec, unsigned int count)
{
	unsigned int i;
	for (i = 0; i < count; i++) {
		print_header(&rec[i].hdr);
		printf("\n");
	}
}
