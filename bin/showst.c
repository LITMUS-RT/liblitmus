#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/mman.h>
#include <fcntl.h>
#include <unistd.h>

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


static void show(char* file)
{
	size_t s;
	struct st_event_record *rec, *end;
	if (map_trace(file, &rec, &end, &s) == 0) {
		print_all(rec, 
			  ((unsigned int)((char*) end - (char*) rec)) 
			  / sizeof(struct st_event_record));
	} else
		perror("mmap");
}

int main(int argc, char** argv)
{
	int i;
	for (i = 1; i < argc; i++)
		show(argv[i]);
	return 0;
}
