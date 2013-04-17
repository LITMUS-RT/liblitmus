#include <sys/mman.h>
#include <sys/fcntl.h> /* for O_RDWR */
#include <sys/unistd.h>
#include <sched.h> /* for sched_yield() */


#include <stdio.h>

#include "litmus.h"
#include "internal.h"

#define LITMUS_CTRL_DEVICE "/dev/litmus/ctrl"
#define CTRL_PAGES 1

#define LITMUS_STATS_FILE "/proc/litmus/stats"

static int map_file(const char* filename, void **addr, size_t size)
{
	int error = 0;
	int fd;


	if (size > 0) {
		fd = open(filename, O_RDWR);
		if (fd >= 0) {
			*addr = mmap(NULL, size,
				     PROT_READ | PROT_WRITE,
				     MAP_PRIVATE,
				     fd, 0);
			if (*addr == MAP_FAILED)
				error = -1;
			close(fd);
		} else
			error = fd;
	} else
		*addr = NULL;
	return error;
}

ssize_t read_file(const char* fname, void* buf, size_t maxlen)
{
	int fd;
	ssize_t n = 0;
	size_t got = 0;

	fd = open(fname, O_RDONLY);
	if (fd == -1)
		return -1;

	while (got < maxlen && (n = read(fd, buf + got, maxlen - got)) > 0)
		got += n;
	close(fd);
	if (n < 0)
		return -1;
	else
		return got;
}

int read_litmus_stats(int *ready, int *all)
{
	char buf[100];
	ssize_t len;

	len = read_file(LITMUS_STATS_FILE, buf, sizeof(buf) - 1);
	if (len >= 0)
		len = sscanf(buf,
			     "real-time tasks   = %d\n"
			     "ready for release = %d\n",
			     all, ready);
	return len == 2;
}

int get_nr_ts_release_waiters(void)
{
	int ready, all;
	if (read_litmus_stats(&ready, &all))
		return ready;
	else
		return -1;
}

/* thread-local pointer to control page */
static __thread struct control_page *ctrl_page;

int init_kernel_iface(void)
{
	int err = 0;
	long page_size = sysconf(_SC_PAGESIZE);
	void* mapped_at = NULL;

	BUILD_BUG_ON(sizeof(union np_flag) != sizeof(uint64_t));

	BUILD_BUG_ON(offsetof(struct control_page, sched.raw)
		     != LITMUS_CP_OFFSET_SCHED);
	BUILD_BUG_ON(offsetof(struct control_page, irq_count)
		     != LITMUS_CP_OFFSET_IRQ_COUNT);
	BUILD_BUG_ON(offsetof(struct control_page, ts_syscall_start)
		     != LITMUS_CP_OFFSET_TS_SC_START);
	BUILD_BUG_ON(offsetof(struct control_page, irq_syscall_start)
		     != LITMUS_CP_OFFSET_IRQ_SC_START);

	err = map_file(LITMUS_CTRL_DEVICE, &mapped_at, CTRL_PAGES * page_size);

	/* Assign ctrl_page indirectly to avoid GCC warnings about aliasing
	 * related to type pruning.
	 */
	ctrl_page = mapped_at;

	if (err) {
		fprintf(stderr, "%s: cannot open LITMUS^RT control page (%m)\n",
			__FUNCTION__);
	}

	return err;
}

void enter_np(void)
{
	if (likely(ctrl_page != NULL) || init_kernel_iface() == 0)
		ctrl_page->sched.np.flag++;
	else
		fprintf(stderr, "enter_np: control page not mapped!\n");
}


void exit_np(void)
{
	if (likely(ctrl_page != NULL) &&
	    ctrl_page->sched.np.flag &&
	    !(--ctrl_page->sched.np.flag)) {
		/* became preemptive, let's check for delayed preemptions */
		__sync_synchronize();
		if (ctrl_page->sched.np.preempt)
			sched_yield();
	}
}

int requested_to_preempt(void)
{
	return (likely(ctrl_page != NULL) && ctrl_page->sched.np.preempt);
}

/* init and return a ptr to the control page for
 * preemption and migration overhead analysis
 *
 * FIXME it may be desirable to have a RO control page here
 */
struct control_page* get_ctrl_page(void)
{
	if((ctrl_page != NULL) || init_kernel_iface() == 0)
		return ctrl_page;
	else
		return NULL;
}

