#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <time.h>
#include <sched.h>
#include <assert.h>
#include <string.h>
#include <stdint.h>
#include <sys/fcntl.h>
#include <sys/mman.h>
#include <inttypes.h>

/* Test tool for validating Litmus's uncache device.     */
/* Tool also capable basic cache vs. sysmem statistics.  */
/* Compile with '-O2' for significaintly greater margins */
/* in performance between cache and sysmem:              */
/* (Intel Xeon X5650)                                    */
/*    -g -> uncache is 30x slower                        */
/*    -O2 -> uncache is >100x slower                     */

int PAGE_SIZE;
#define NR_PAGES 16

#define UNCACHE_DEV "/dev/litmus/uncache"

/* volatile forces a read from memory (or cache) on every reference. Note
   that volatile does not keep data out of the cache! */
typedef volatile char* pbuf_t;

/* hit the first byte in each page.
   addr must be page aligned. */
inline int linear_write(pbuf_t addr, int size, char val)
{
	pbuf_t end = addr + size;
	pbuf_t step;
	int nr_pages = (unsigned long)(end - addr)/PAGE_SIZE;
	int times = nr_pages * PAGE_SIZE;
	int i;

	for (i = 0; i < times; ++i)
		for(step = addr; step < end; step += PAGE_SIZE)
			*step = val;
	return 0;
}
inline int linear_read(pbuf_t addr, int size, char val)
{
	pbuf_t end = addr + size;
	pbuf_t step;
	int nr_pages = (unsigned long)(end - addr)/PAGE_SIZE;
	int times = nr_pages * PAGE_SIZE;
	int i;

	for (i = 0; i < times; ++i)
		for(step = addr; step < end; step += PAGE_SIZE) {
			if (*step != val)
				return -1;
		}
	return 0;
}

/* write to *data nr times. */
inline int hammer_write(pbuf_t data, char val, int nr)
{
	int i;
	for (i = 0; i < nr; ++i)
		*data = val;
	return 0;
}

/* read from *data nr times. */
inline int hammer_read(pbuf_t data, char val, int nr)
{
	int i;
	for (i = 0; i < nr; ++i) {
		if (*data != val)
			return -1;
	}
	return 0;
}

inline int test(pbuf_t data, int size, int trials)
{
	int HAMMER_TIME = 10000;  /* can't cache this! */
	char VAL = 0x55;
	int t;
	for(t = 0; t < trials; ++t) {

#if 0
		if (linear_write(data, size, VAL) != 0) {
			printf("failed linear_write()\n");
			return -1;
		}
		if (linear_read(data, size, VAL) != 0) {
			printf("failed linear_read()\n");
			return -1;
		}
#endif

		/* hammer at the first byte in the array */
		if (hammer_write(data, VAL, HAMMER_TIME) != 0) {
			printf("failed hammer_write()\n");
			return -1;
		}
		if (hammer_read(data, VAL, HAMMER_TIME) != 0) {
			printf("failed hammer_read()\n");
			return -1;
		}
	}
	return 0;
}

inline void timespec_normalize(struct timespec* ts, time_t sec, int64_t nsec)
{
	while(nsec > 1000000000LL) {
		asm("" : "+rm"(nsec));
		nsec -= 1000000000LL;
		++sec;
	}
	while(nsec < 0) {
		asm("" : "+rm"(nsec));
		nsec += 1000000000LL;
		--sec;
	}

	ts->tv_sec = sec;
	ts->tv_nsec = nsec;
}

inline struct timespec timespec_sub(struct timespec lhs, struct timespec rhs)
{
	struct timespec delta;
	timespec_normalize(&delta, lhs.tv_sec - rhs.tv_sec, lhs.tv_nsec - rhs.tv_nsec);
	return delta;
}

inline struct timespec timespec_add(struct timespec lhs, struct timespec rhs)
{
	struct timespec delta;
	timespec_normalize(&delta, lhs.tv_sec + rhs.tv_sec, lhs.tv_nsec + rhs.tv_nsec);
	return delta;
}

inline int64_t timespec_to_us(struct timespec ts)
{
	int64_t t;
	t = ts.tv_sec * 1000000LL;
	t += ts.tv_nsec / 1000LL;
	return t;
}

/* hammers away at the first byte in each mmaped page and
   times how long it took. */
int do_data(int do_uncache, int64_t* time)
{
	int size;
	int prot = PROT_READ | PROT_WRITE;
	int flags = MAP_PRIVATE;

	pbuf_t data;

	struct sched_param fifo_params;

	struct timespec start, end;
	int64_t elapsed;
	int trials = 1000;

	printf("Running data access test.\n");

	mlockall(MCL_CURRENT | MCL_FUTURE);

	memset(&fifo_params, 0, sizeof(fifo_params));
	fifo_params.sched_priority = sched_get_priority_max(SCHED_FIFO);

	size = PAGE_SIZE*NR_PAGES;

	printf("Allocating %d %s pages.\n", NR_PAGES, (do_uncache) ?
					"uncacheable" : "cacheable");
	if (do_uncache) {
		int fd = open(UNCACHE_DEV, O_RDWR);
		data = mmap(NULL, size, prot, flags, fd, 0);
		close(fd);
	}
	else {
		/* Accessed data will probably fit in L1, so this will go VERY fast.
		   Code should also have little-to-no pipeline stalls. */
		flags |= MAP_ANONYMOUS;
		data = mmap(NULL, size, prot, flags, -1, 0);
	}
	if (data == MAP_FAILED) {
		printf("Failed to alloc data! "
			   "Are you running Litmus? "
			   "Is Litmus broken?\n");
		return -1;
	}
	else {
		printf("Data allocated at %p.\n", data);
	}

	printf("Beginning tests...\n");
	if (sched_setscheduler(getpid(), SCHED_FIFO, &fifo_params)) {
		printf("(Could not become SCHED_FIFO task.) Are you running as root?\n");
	}

	/* observations suggest that no warmup phase is needed. */
	clock_gettime(CLOCK_THREAD_CPUTIME_ID, &start);
	if (test(data, size, trials) != 0) {
		printf("Test failed!\n");
		munmap((char*)data, size);
		return -1;
	}
	clock_gettime(CLOCK_THREAD_CPUTIME_ID, &end);
	elapsed = timespec_to_us(timespec_sub(end, start));
	printf("%s Time: %"PRIi64"us\n", (do_uncache) ?
					"Uncache" : "Cache", elapsed);

	munmap((char*)data, size);

	if(time)
		*time = elapsed;

	return 0;
}

/* compares runtime of cached vs. uncached */
int do_data_compare()
{
	const double thresh = 1.3;
	int ret = 0;
	double ratio;
	int64_t cache_time = 0, uncache_time = 0;

	printf("Timing cached pages...\n");
	ret = do_data(0, &cache_time);
	if (ret != 0)
		goto out;

	printf("Timing uncached pages...\n");
	ret = do_data(1, &uncache_time);
	if (ret != 0)
		goto out;

	ratio = (double)uncache_time/(double)cache_time;
	printf("Uncached/Cached Ratio: %f\n", ratio);

	if (ratio < thresh) {
		printf("Ratio is unexpectedly small (< %f)! "
				" Uncache broken? Are you on kvm?\n", thresh);
		ret = -1;
	}

out:
	return ret;
}

/* tries to max out uncache allocations.
   under normal conditions (non-mlock),
   pages should spill into swap. uncache
   pages are not locked in memory. */
int do_max_alloc(void)
{
	int fd;
	int good = 1;
	int count = 0;
	uint64_t mmap_size = PAGE_SIZE; /* start at one page per mmap */

	/* half of default limit on ubuntu. (see /proc/sys/vm/max_map_count) */
	int max_mmaps = 32765;
	volatile char** maps = calloc(max_mmaps, sizeof(pbuf_t));

	if (!maps) {
		printf("failed to alloc pointers for pages\n");
		return -1;
	}

	printf("Testing max amount of uncache data. System may get wonkie (OOM Killer)!\n");

	fd = open(UNCACHE_DEV, O_RDWR);
	do {
		int i;
		int nr_pages = mmap_size/PAGE_SIZE;
		printf("Testing mmaps of %d pages.\n", nr_pages);

		count = 0;
		for (i = 0; (i < max_mmaps) && good; ++i) {
			pbuf_t data = mmap(NULL, mmap_size, PROT_READ | PROT_WRITE, MAP_PRIVATE | MAP_POPULATE, fd, 0);

			if (data != MAP_FAILED) {
				maps[i] = data;
				++count;
			}
			else {
				perror(NULL);
				good = 0;
			}
		}
		for (i = 0; i < count; ++i) {
			if (maps[i])
				munmap((char*)(maps[i]), mmap_size);
		}
		memset(maps, 0, sizeof(maps[0])*max_mmaps);

		mmap_size *= 2; /* let's do it again with bigger allocations */
	}while(good);

	free(maps);
	close(fd);

	printf("Maxed out allocs with %d mmaps of %"PRIu64" pages in size.\n",
		count, mmap_size/PAGE_SIZE);

	return 0;
}

typedef enum
{
	UNCACHE,
	CACHE,
	COMPARE,
	MAX_ALLOC
} test_t;

#define OPTSTR "ucxa"
int main(int argc, char** argv)
{
	int ret;
	test_t test = UNCACHE;
	int opt;
	PAGE_SIZE = sysconf(_SC_PAGE_SIZE);

	while((opt = getopt(argc, argv, OPTSTR)) != -1) {
		switch(opt) {
			case 'c':
				test = CACHE;
				break;
			case 'u':
				test = UNCACHE;
				break;
			case 'x':
				test = COMPARE;
				break;
			case 'a':
				test = MAX_ALLOC;
				break;
			case ':':
				printf("missing option\n");
				exit(-1);
			case '?':
			default:
				printf("bad argument\n");
				exit(-1);
		}
	}


	printf("Page Size: %d\n", PAGE_SIZE);

	switch(test)
	{
	case CACHE:
		ret = do_data(0, NULL);
		break;
	case UNCACHE:
		ret = do_data(1, NULL);
		break;
	case COMPARE:
		ret = do_data_compare();
		break;
	case MAX_ALLOC:
		ret = do_max_alloc();
		break;
	default:
		printf("invalid test\n");
		ret = -1;
		break;
	}

	if (ret != 0) {
		printf("Test failed.\n");
	}

	return ret;
}
