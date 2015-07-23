#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <unistd.h>
#include <errno.h>

#include <sched.h>

#include "litmus.h"
#include "internal.h"

#define SCHED_NORMAL 0

int task_mode(int mode)
{
	struct sched_param param;
	int me     = gettid();
	int policy = sched_getscheduler(gettid());
	int old_mode = policy == SCHED_LITMUS ? LITMUS_RT_TASK : BACKGROUND_TASK;

	memset(&param, 0, sizeof(param));
	param.sched_priority = 0;
	if (old_mode == LITMUS_RT_TASK && mode == BACKGROUND_TASK) {
		/* transition to normal task */
		return sched_setscheduler(me, SCHED_NORMAL, &param);
	} else if (old_mode == BACKGROUND_TASK && mode == LITMUS_RT_TASK) {
		/* transition to RT task */
		return sched_setscheduler(me, SCHED_LITMUS, &param);
	} else {
		errno = -EINVAL;
		return -1;
	}
}
