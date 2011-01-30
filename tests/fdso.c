#include <sys/wait.h> /* for waitpid() */

#include <fcntl.h>
#include <unistd.h>

#include <stdio.h>


#include "tests.h"

#include "litmus.h"


TESTCASE(fmlp_not_active, C_EDF | PFAIR | LINUX,
	 "don't open FMLP semaphores if FMLP is not supported")
{
	int fd;

	SYSCALL( fd = open(".fmlp_locks", O_RDONLY | O_CREAT) );

	ASSERT(fd != -1);

	SYSCALL_FAILS(ENXIO, open_fmlp_sem(fd, 0) );

	SYSCALL( close(fd) );

	SYSCALL( remove(".fmlp_locks") );
}


TESTCASE(invalid_od, ALL,
	 "reject invalid object descriptors")
{
	SYSCALL_FAILS( EINVAL, litmus_lock(3) );

	SYSCALL_FAILS( EINVAL, litmus_unlock(3) );

	SYSCALL_FAILS( EINVAL, od_close(3) );


	SYSCALL_FAILS( EINVAL, litmus_lock(-1) );

	SYSCALL_FAILS( EINVAL, litmus_unlock(-1) );

	SYSCALL_FAILS( EINVAL, od_close(-1) );
}

TESTCASE(invalid_obj_type, ALL,
	 "reject invalid object types")
{
	SYSCALL_FAILS( EINVAL, od_open(0, -1, 0) );
	SYSCALL_FAILS( EINVAL, od_open(0, 10, 0) );
}

TESTCASE(not_inherit_od, GSN_EDF | PSN_EDF,
	 "don't inherit FDSO handles across fork")
{
	int fd, od, pid, status;

	SYSCALL( fd = open(".fmlp_locks", O_RDONLY | O_CREAT) );

	SYSCALL( od = open_fmlp_sem(fd, 0) );

	pid = fork();

	ASSERT( pid != -1 );

	/* must be an RT task to lock at all */
	SYSCALL( sporadic_partitioned(10, 100, 0) );
	SYSCALL( task_mode(LITMUS_RT_TASK) );

	if (pid == 0) {
		/* child */
		SYSCALL_FAILS(EINVAL, litmus_lock(od) );
	        SYSCALL_FAILS(EINVAL, litmus_unlock(od) );

		SYSCALL( task_mode(BACKGROUND_TASK) );

		exit(0);
	} else {

		SYSCALL( litmus_lock(od) );
		SYSCALL( litmus_unlock(od) );

		SYSCALL( litmus_lock(od) );
		SYSCALL( litmus_unlock(od) );

		SYSCALL( task_mode(BACKGROUND_TASK) );

		SYSCALL( waitpid(pid, &status, 0) );

		ASSERT(WEXITSTATUS(status) == 0);
	}

	SYSCALL( od_close(od) );

	SYSCALL( close(fd) );

	SYSCALL( remove(".fmlp_locks") );
}


