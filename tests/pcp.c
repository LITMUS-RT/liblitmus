#include <fcntl.h>
#include <unistd.h>
#include <stdio.h>

#include "tests.h"
#include "litmus.h"


TESTCASE(lock_pcp, P_FP,
	 "PCP acquisition and release")
{
	int fd, od, cpu = 0;

	SYSCALL( fd = open(".pcp_locks", O_RDONLY | O_CREAT) );

	SYSCALL( sporadic_partitioned(10, 100, cpu) );
	SYSCALL( task_mode(LITMUS_RT_TASK) );

	SYSCALL( od = open_pcp_sem(fd, 0, cpu) );

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

	SYSCALL( remove(".pcp_locks") );
}

TESTCASE(lock_dpcp, P_FP,
	 "DPCP acquisition and release")
{
	int fd, od, cpu = 1;

	SYSCALL( fd = open(".pcp_locks", O_RDONLY | O_CREAT) );

	SYSCALL( sporadic_partitioned(10, 100, 0) );
	SYSCALL( task_mode(LITMUS_RT_TASK) );

	SYSCALL( od = open_dpcp_sem(fd, 0, cpu) );

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

	SYSCALL( remove(".pcp_locks") );
}

TESTCASE(not_lock_pcp_be, P_FP,
	 "don't let best-effort tasks lock (D|M-)PCP semaphores")
{
	int fd, od;

	SYSCALL( fd = open(".pcp_locks", O_RDONLY | O_CREAT) );

	/* BE tasks are not even allowed to open a PCP semaphore */
	SYSCALL_FAILS(EPERM, od = open_pcp_sem(fd, 0, 1) );

	/* BE tasks are not allowed to open a D-PCP semaphore */
	SYSCALL_FAILS(EPERM, od = open_dpcp_sem(fd, 0, 1) );

	/* BE tasks are not allowed to open an M-PCP semaphore */
	SYSCALL_FAILS(EPERM, od = open_mpcp_sem(fd, 0) );

	SYSCALL( close(fd) );

	SYSCALL( remove(".pcp_locks") );

}

TESTCASE(lock_mpcp, P_FP,
	 "MPCP acquisition and release")
{
	int fd, od;

	SYSCALL( fd = open(".pcp_locks", O_RDONLY | O_CREAT) );

	SYSCALL( sporadic_partitioned(10, 100, 0) );
	SYSCALL( task_mode(LITMUS_RT_TASK) );

	SYSCALL( od = open_mpcp_sem(fd, 0) );

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

	SYSCALL( remove(".pcp_locks") );
}
