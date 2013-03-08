#include <fcntl.h>
#include <unistd.h>
#include <stdio.h>

#include "tests.h"
#include "litmus.h"


TESTCASE(not_lock_fmlp_be, GSN_EDF | PSN_EDF | P_FP,
	 "don't let best-effort tasks lock FMLP semaphores")
{
	int fd, od;

	SYSCALL( fd = open(".fmlp_locks", O_RDONLY | O_CREAT, S_IRUSR) );

	SYSCALL( od = open_fmlp_sem(fd, 0) );

	/* BE tasks may not lock FMLP semaphores */
	SYSCALL_FAILS(EPERM, litmus_lock(od) );

	/* tasks may not unlock resources they don't own */
	SYSCALL_FAILS(EINVAL, litmus_unlock(od) );

	SYSCALL( od_close(od) );

	SYSCALL( close(fd) );

	SYSCALL( remove(".fmlp_locks") );

}

TESTCASE(not_lock_srp_be, PSN_EDF | P_FP,
	 "don't let best-effort tasks open SRP semaphores")
{
	int fd, od;

	SYSCALL( fd = open(".srp_locks", O_RDONLY | O_CREAT, S_IRUSR) );

	/* BE tasks may not open SRP semaphores */

	SYSCALL_FAILS(EPERM, od = open_srp_sem(fd, 0) );

	SYSCALL( close(fd) );

	SYSCALL( remove(".srp_locks") );

}

TESTCASE(lock_srp, PSN_EDF | P_FP,
	 "SRP acquisition and release")
{
	int fd, od;

	SYSCALL( fd = open(".srp_locks", O_RDONLY | O_CREAT, S_IRUSR) );

	SYSCALL( sporadic_partitioned(10, 100, 0) );
	SYSCALL( task_mode(LITMUS_RT_TASK) );

	SYSCALL( od = open_srp_sem(fd, 0) );

	SYSCALL( litmus_lock(od) );
	SYSCALL( litmus_unlock(od) );

	SYSCALL( litmus_lock(od) );
	SYSCALL( litmus_unlock(od) );

	SYSCALL( litmus_lock(od) );
	SYSCALL( litmus_unlock(od) );

	/* tasks may not unlock resources they don't own */
	SYSCALL_FAILS(EINVAL, litmus_unlock(od) );

	SYSCALL( od_close(od) );

	SYSCALL( close(fd) );

	SYSCALL( remove(".srp_locks") );
}


TESTCASE(lock_fmlp, PSN_EDF | GSN_EDF | P_FP,
	 "FMLP acquisition and release")
{
	int fd, od;

	SYSCALL( fd = open(".fmlp_locks", O_RDONLY | O_CREAT, S_IRUSR) );

	SYSCALL( sporadic_partitioned(10, 100, 0) );
	SYSCALL( task_mode(LITMUS_RT_TASK) );

	SYSCALL( od = open_fmlp_sem(fd, 0) );

	SYSCALL( litmus_lock(od) );
	SYSCALL( litmus_unlock(od) );

	SYSCALL( litmus_lock(od) );
	SYSCALL( litmus_unlock(od) );

	SYSCALL( litmus_lock(od) );
	SYSCALL( litmus_unlock(od) );

	/* tasks may not unlock resources they don't own */
	SYSCALL_FAILS(EINVAL, litmus_unlock(od) );

	SYSCALL( od_close(od) );

	SYSCALL( close(fd) );

	SYSCALL( remove(".fmlp_locks") );
}
