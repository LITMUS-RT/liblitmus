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
	printf("%-14s %5u/%-5u on CPU%3u ",
	       event2name(hdr->type),
	       hdr->pid, hdr->job,
	       hdr->cpu);
}

typedef void (*print_t)(struct st_event_record* rec);

static void print_nothing(struct st_event_record* _)
{
}

static void print_name(struct st_event_record* rec)
{
	/* terminate in all cases */
	rec->data.name.cmd[ST_NAME_LEN - 1] = 0;
	printf("%s", rec->data.name.cmd);
}

static void print_param(struct st_event_record* rec)
{
	printf("T=(cost:%6.2fms, period:%6.2fms, phase:%6.2fms), part=%d",
	       rec->data.param.wcet   / 1000000.0,
	       rec->data.param.period / 1000000.0,
	       rec->data.param.phase  / 1000000.0,\
	       rec->data.param.partition);
}

static print_t print_detail[] = {
	print_nothing,
	print_name,
	print_param,
	print_nothing,
	print_nothing,
	print_nothing,
	print_nothing,
	print_nothing,
	print_nothing,
	print_nothing,
	print_nothing
};

void print_event(struct st_event_record *rec)
{
	unsigned int id = rec->hdr.type;

	if (id >= ST_INVALID)
		id = ST_INVALID;
	print_header(&rec->hdr);
	print_detail[id](rec);
	printf("\n");
	return event_names[id];
}


void print_all(struct st_event_record *rec, unsigned int count)
{
	unsigned int i;
	for (i = 0; i < count; i++)
		print_event(&rec[i]);
}
