#include <sys/mman.h>
#include <sys/fcntl.h> /* for O_RDWR */
#include <sys/unistd.h>
#include <sched.h> /* for sched_yield() */


#include <stdio.h>

#include "litmus.h"
#include "internal.h"

#define LITMUS_CTRL_DEVICE "/dev/litmus/ctrl"
#define CTRL_PAGES 1

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

/* thread-local pointer to control page */
static __thread struct control_page *ctrl_page;

int init_kernel_iface(void)
{
	int err = 0;
	long page_size = sysconf(_SC_PAGESIZE);

	BUILD_BUG_ON(sizeof(union np_flag) != sizeof(uint32_t));

	err = map_file(LITMUS_CTRL_DEVICE, (void**) &ctrl_page, CTRL_PAGES * page_size);
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

