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

	SYSCALL_FAILS(EBUSY, open_fmlp_sem(fd, 0) );

	SYSCALL( close(fd) );

	SYSCALL( remove(".fmlp_locks") );
}


TESTCASE(invalid_od, ALL,
	 "reject invalid object descriptors")
{
	SYSCALL_FAILS( EINVAL, fmlp_down(3) );

	SYSCALL_FAILS( EINVAL, fmlp_up(3) );

	SYSCALL_FAILS( EINVAL, od_close(3) );


	SYSCALL_FAILS( EINVAL, fmlp_down(-1) );

	SYSCALL_FAILS( EINVAL, fmlp_up(-1) );

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

	SYSCALL( fmlp_down(od) );

	SYSCALL( fmlp_up(od) );

	pid = fork();

	ASSERT( pid != -1 );

	if (pid == 0) {
		/* child */
		SYSCALL_FAILS(EINVAL, fmlp_down(od) );
	        SYSCALL_FAILS(EINVAL, fmlp_up(od) );
		exit(0);
	} else {
		SYSCALL( fmlp_down(od) );
		SYSCALL( fmlp_up(od) );
		SYSCALL( waitpid(pid, &status, 0) );
		ASSERT(WEXITSTATUS(status) == 0);
	}

	SYSCALL( od_close(od) );

	SYSCALL( close(fd) );

	SYSCALL( remove(".fmlp_locks") );
}


