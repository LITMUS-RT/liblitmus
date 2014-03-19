#include <fcntl.h>
#include <unistd.h>
#include <stdio.h>

#include "tests.h"
#include "litmus.h"

TESTCASE(lock_fmlp_nesting, PSN_EDF | GSN_EDF | P_FP,
	 "FMLP no nesting allowed")
{
	int fd, od, od2;

	SYSCALL( fd = open(".fmlp_locks", O_RDONLY | O_CREAT, S_IRUSR) );

	SYSCALL( sporadic_partitioned(10, 100, 0) );
	SYSCALL( task_mode(LITMUS_RT_TASK) );

	SYSCALL( od = open_fmlp_sem(fd, 0) );
	SYSCALL( od2 = open_fmlp_sem(fd, 1) );

	SYSCALL( litmus_lock(od) );
	SYSCALL( litmus_unlock(od) );

	SYSCALL( litmus_lock(od2) );
	SYSCALL( litmus_unlock(od2) );

	SYSCALL( litmus_lock(od) );
	SYSCALL_FAILS(EBUSY, litmus_lock(od2));
	SYSCALL( litmus_unlock(od) );

	SYSCALL( litmus_lock(od2) );
	SYSCALL_FAILS(EBUSY, litmus_lock(od));
	SYSCALL( litmus_unlock(od2) );

	SYSCALL( od_close(od) );
	SYSCALL( od_close(od2) );

	SYSCALL( close(fd) );

	SYSCALL( remove(".fmlp_locks") );
}

TESTCASE(lock_fmlp_srp_nesting, PSN_EDF | P_FP,
	 "FMLP no nesting with SRP resources allowed")
{
	int fd, od, od2;

	SYSCALL( fd = open(".fmlp_locks", O_RDONLY | O_CREAT, S_IRUSR) );

	SYSCALL( sporadic_partitioned(10, 100, 0) );
	SYSCALL( task_mode(LITMUS_RT_TASK) );

	SYSCALL( od = open_fmlp_sem(fd, 0) );
	SYSCALL( od2 = open_srp_sem(fd, 1) );

	SYSCALL( litmus_lock(od) );
	SYSCALL( litmus_unlock(od) );

	SYSCALL( litmus_lock(od2) );
	SYSCALL( litmus_unlock(od2) );

	SYSCALL( litmus_lock(od) );
	SYSCALL_FAILS(EBUSY, litmus_lock(od2));
	SYSCALL( litmus_unlock(od) );

	SYSCALL( litmus_lock(od2) );
	SYSCALL_FAILS(EBUSY, litmus_lock(od));
	SYSCALL( litmus_unlock(od2) );

	SYSCALL( od_close(od) );
	SYSCALL( od_close(od2) );

	SYSCALL( close(fd) );

	SYSCALL( remove(".fmlp_locks") );
}

TESTCASE(lock_srp_nesting, PSN_EDF | P_FP,
	 "SRP nesting allowed")
{
	int fd, od, od2;

	SYSCALL( fd = open(".fmlp_locks", O_RDONLY | O_CREAT, S_IRUSR) );

	SYSCALL( sporadic_partitioned(10, 100, 0) );
	SYSCALL( task_mode(LITMUS_RT_TASK) );

	SYSCALL( od = open_srp_sem(fd, 0) );
	SYSCALL( od2 = open_srp_sem(fd, 1) );

	SYSCALL( litmus_lock(od) );
	SYSCALL( litmus_unlock(od) );

	SYSCALL( litmus_lock(od2) );
	SYSCALL( litmus_unlock(od2) );

	SYSCALL( litmus_lock(od) );
	SYSCALL( litmus_lock(od2) );
	SYSCALL( litmus_unlock(od2) );
	SYSCALL( litmus_unlock(od) );

	SYSCALL( litmus_lock(od2) );
	SYSCALL( litmus_lock(od) );
	SYSCALL( litmus_unlock(od) );
	SYSCALL( litmus_unlock(od2) );

	SYSCALL( od_close(od) );
	SYSCALL( od_close(od2) );

	SYSCALL( close(fd) );

	SYSCALL( remove(".fmlp_locks") );
}

TESTCASE(lock_pcp_nesting, P_FP,
	 "PCP nesting allowed")
{
	int od, od2;
	const char* namespace = ".pcp_locks";

	SYSCALL( sporadic_partitioned(10, 100, 0) );
	SYSCALL( task_mode(LITMUS_RT_TASK) );

	SYSCALL( od  = litmus_open_lock(PCP_SEM, 0, namespace, NULL) );
	SYSCALL( od2 = litmus_open_lock(PCP_SEM, 1, namespace, NULL) );

	SYSCALL( litmus_lock(od) );
	SYSCALL( litmus_unlock(od) );

	SYSCALL( litmus_lock(od2) );
	SYSCALL( litmus_unlock(od2) );

	SYSCALL( litmus_lock(od) );
	SYSCALL( litmus_lock(od2) );
	SYSCALL( litmus_unlock(od2) );
	SYSCALL( litmus_unlock(od) );

	SYSCALL( litmus_lock(od2) );
	SYSCALL( litmus_lock(od) );
	SYSCALL( litmus_unlock(od) );
	SYSCALL( litmus_unlock(od2) );

	SYSCALL( od_close(od) );
	SYSCALL( od_close(od2) );

	SYSCALL( remove(namespace) );
}

TESTCASE(lock_mpcp_pcp_no_nesting, P_FP,
	 "PCP and MPCP nesting not allowed")
{
	int od, od2;
	const char* namespace = ".pcp_locks";

	SYSCALL( sporadic_partitioned(10, 100, 0) );
	SYSCALL( task_mode(LITMUS_RT_TASK) );

	SYSCALL( od  = litmus_open_lock(PCP_SEM, 0, namespace, NULL) );
	SYSCALL( od2 = litmus_open_lock(MPCP_SEM, 1, namespace, NULL) );

	SYSCALL( litmus_lock(od) );
	SYSCALL( litmus_unlock(od) );

	SYSCALL( litmus_lock(od2) );
	SYSCALL( litmus_unlock(od2) );

	SYSCALL( litmus_lock(od) );
	SYSCALL_FAILS(EBUSY, litmus_lock(od2));
	SYSCALL( litmus_unlock(od) );

	SYSCALL( litmus_lock(od2) );
	SYSCALL_FAILS(EBUSY, litmus_lock(od));
	SYSCALL( litmus_unlock(od2) );

	SYSCALL( od_close(od) );
	SYSCALL( od_close(od2) );

	SYSCALL( remove(namespace) );
}

TESTCASE(lock_fmlp_pcp_no_nesting, P_FP,
	 "PCP and FMLP nesting not allowed")
{
	int od, od2;
	const char* namespace = ".pcp_locks";

	SYSCALL( sporadic_partitioned(10, 100, 0) );
	SYSCALL( task_mode(LITMUS_RT_TASK) );

	SYSCALL( od  = litmus_open_lock(PCP_SEM, 0, namespace, NULL) );
	SYSCALL( od2 = litmus_open_lock(FMLP_SEM, 1, namespace, NULL) );

	SYSCALL( litmus_lock(od) );
	SYSCALL( litmus_unlock(od) );

	SYSCALL( litmus_lock(od2) );
	SYSCALL( litmus_unlock(od2) );

	SYSCALL( litmus_lock(od) );
	SYSCALL_FAILS(EBUSY, litmus_lock(od2));
	SYSCALL( litmus_unlock(od) );

	SYSCALL( litmus_lock(od2) );
	SYSCALL_FAILS(EBUSY, litmus_lock(od));
	SYSCALL( litmus_unlock(od2) );

	SYSCALL( od_close(od) );
	SYSCALL( od_close(od2) );

	SYSCALL( remove(namespace) );
}

TESTCASE(lock_dpcp_pcp_no_nesting, P_FP,
	 "PCP and DPCP nesting not allowed")
{
	int od, od2;
	int cpu = 0;
	const char* namespace = ".pcp_locks";

	SYSCALL( sporadic_partitioned(10, 100, 0) );
	SYSCALL( task_mode(LITMUS_RT_TASK) );

	SYSCALL( od  = litmus_open_lock(PCP_SEM, 0, namespace, NULL) );
	SYSCALL( od2 = litmus_open_lock(DPCP_SEM, 1, namespace, &cpu) );

	SYSCALL( litmus_lock(od) );
	SYSCALL( litmus_unlock(od) );

	SYSCALL( litmus_lock(od2) );
	SYSCALL( litmus_unlock(od2) );

	SYSCALL( litmus_lock(od) );
	SYSCALL_FAILS(EBUSY, litmus_lock(od2));
	SYSCALL( litmus_unlock(od) );

	SYSCALL( litmus_lock(od2) );
	SYSCALL_FAILS(EBUSY, litmus_lock(od));
	SYSCALL( litmus_unlock(od2) );

	SYSCALL( od_close(od) );
	SYSCALL( od_close(od2) );

	SYSCALL( remove(namespace) );
}

TESTCASE(lock_mpcp_srp_no_nesting, P_FP,
	 "SRP and MPCP nesting not allowed")
{
	int od, od2;
	const char* namespace = ".pcp_locks";

	SYSCALL( sporadic_partitioned(10, 100, 0) );
	SYSCALL( task_mode(LITMUS_RT_TASK) );

	SYSCALL( od  = litmus_open_lock(SRP_SEM, 0, namespace, NULL) );
	SYSCALL( od2 = litmus_open_lock(MPCP_SEM, 1, namespace, NULL) );

	SYSCALL( litmus_lock(od) );
	SYSCALL( litmus_unlock(od) );

	SYSCALL( litmus_lock(od2) );
	SYSCALL( litmus_unlock(od2) );

	SYSCALL( litmus_lock(od) );
	SYSCALL_FAILS(EBUSY, litmus_lock(od2));
	SYSCALL( litmus_unlock(od) );

	SYSCALL( litmus_lock(od2) );
	SYSCALL_FAILS(EBUSY, litmus_lock(od));
	SYSCALL( litmus_unlock(od2) );

	SYSCALL( od_close(od) );
	SYSCALL( od_close(od2) );

	SYSCALL( remove(namespace) );
}

TESTCASE(lock_dpcp_srp_no_nesting, P_FP,
	 "SRP and DPCP nesting not allowed")
{
	int od, od2;
	int cpu = 0;
	const char* namespace = ".pcp_locks";

	SYSCALL( sporadic_partitioned(10, 100, 0) );
	SYSCALL( task_mode(LITMUS_RT_TASK) );

	SYSCALL( od  = litmus_open_lock(SRP_SEM, 0, namespace, NULL) );
	SYSCALL( od2 = litmus_open_lock(DPCP_SEM, 1, namespace, &cpu) );

	SYSCALL( litmus_lock(od) );
	SYSCALL( litmus_unlock(od) );

	SYSCALL( litmus_lock(od2) );
	SYSCALL( litmus_unlock(od2) );

	SYSCALL( litmus_lock(od) );
	SYSCALL_FAILS(EBUSY, litmus_lock(od2));
	SYSCALL( litmus_unlock(od) );

	SYSCALL( litmus_lock(od2) );
	SYSCALL_FAILS(EBUSY, litmus_lock(od));
	SYSCALL( litmus_unlock(od2) );

	SYSCALL( od_close(od) );
	SYSCALL( od_close(od2) );

	SYSCALL( remove(namespace) );
}

TESTCASE(lock_fmlp_mpcp_no_nesting, P_FP,
	 "MPCP and FMLP nesting not allowed")
{
	int od, od2;
	const char* namespace = ".pcp_locks";

	SYSCALL( sporadic_partitioned(10, 100, 0) );
	SYSCALL( task_mode(LITMUS_RT_TASK) );

	SYSCALL( od  = litmus_open_lock(MPCP_SEM, 0, namespace, NULL) );
	SYSCALL( od2 = litmus_open_lock(FMLP_SEM, 1, namespace, NULL) );

	SYSCALL( litmus_lock(od) );
	SYSCALL( litmus_unlock(od) );

	SYSCALL( litmus_lock(od2) );
	SYSCALL( litmus_unlock(od2) );

	SYSCALL( litmus_lock(od) );
	SYSCALL_FAILS(EBUSY, litmus_lock(od2));
	SYSCALL( litmus_unlock(od) );

	SYSCALL( litmus_lock(od2) );
	SYSCALL_FAILS(EBUSY, litmus_lock(od));
	SYSCALL( litmus_unlock(od2) );

	SYSCALL( od_close(od) );
	SYSCALL( od_close(od2) );

	SYSCALL( remove(namespace) );
}

TESTCASE(lock_fmlp_dpcp_no_nesting, P_FP,
	 "DPCP and FMLP nesting not allowed")
{
	int od, od2;
	const char* namespace = ".pcp_locks";
	int cpu = 0;

	SYSCALL( sporadic_partitioned(10, 100, 0) );
	SYSCALL( task_mode(LITMUS_RT_TASK) );

	SYSCALL( od  = litmus_open_lock(DPCP_SEM, 0, namespace, &cpu) );
	SYSCALL( od2 = litmus_open_lock(FMLP_SEM, 1, namespace, NULL) );

	SYSCALL( litmus_lock(od) );
	SYSCALL( litmus_unlock(od) );

	SYSCALL( litmus_lock(od2) );
	SYSCALL( litmus_unlock(od2) );

	SYSCALL( litmus_lock(od) );
	SYSCALL_FAILS(EBUSY, litmus_lock(od2));
	SYSCALL( litmus_unlock(od) );

	SYSCALL( litmus_lock(od2) );
	SYSCALL_FAILS(EBUSY, litmus_lock(od));
	SYSCALL( litmus_unlock(od2) );

	SYSCALL( od_close(od) );
	SYSCALL( od_close(od2) );

	SYSCALL( remove(namespace) );
}

TESTCASE(mpcp_nesting, P_FP,
	 "MPCP no nesting allowed")
{
	int od, od2;

	SYSCALL( sporadic_partitioned(10, 100, 0) );
	SYSCALL( task_mode(LITMUS_RT_TASK) );

	SYSCALL( od  = litmus_open_lock(MPCP_SEM, 0, ".mpcp_locks", NULL) );
	SYSCALL( od2 = litmus_open_lock(MPCP_SEM, 1, ".mpcp_locks", NULL) );

	SYSCALL( litmus_lock(od) );
	SYSCALL( litmus_unlock(od) );

	SYSCALL( litmus_lock(od2) );
	SYSCALL( litmus_unlock(od2) );

	SYSCALL( litmus_lock(od) );
	SYSCALL_FAILS(EBUSY, litmus_lock(od2));
	SYSCALL( litmus_unlock(od) );

	SYSCALL( litmus_lock(od2) );
	SYSCALL_FAILS(EBUSY, litmus_lock(od));
	SYSCALL( litmus_unlock(od2) );

	SYSCALL( od_close(od) );
	SYSCALL( od_close(od2) );

	SYSCALL( remove(".mpcp_locks") );
}

TESTCASE(mpcpvs_nesting, P_FP,
	 "MPCP-VS no nesting allowed")
{
	int od, od2;

	SYSCALL( sporadic_partitioned(10, 100, 0) );
	SYSCALL( task_mode(LITMUS_RT_TASK) );

	SYSCALL( od  = litmus_open_lock(MPCP_VS_SEM, 0, ".mpcp_locks", NULL) );
	SYSCALL( od2 = litmus_open_lock(MPCP_VS_SEM, 1, ".mpcp_locks", NULL) );

	SYSCALL( litmus_lock(od) );
	SYSCALL( litmus_unlock(od) );

	SYSCALL( litmus_lock(od2) );
	SYSCALL( litmus_unlock(od2) );

	SYSCALL( litmus_lock(od) );
	SYSCALL_FAILS(EBUSY, litmus_lock(od2));
	SYSCALL( litmus_unlock(od) );

	SYSCALL( litmus_lock(od2) );
	SYSCALL_FAILS(EBUSY, litmus_lock(od));
	SYSCALL( litmus_unlock(od2) );

	SYSCALL( od_close(od) );
	SYSCALL( od_close(od2) );

	SYSCALL( remove(".mpcp_locks") );
}

TESTCASE(dpcp_nesting, P_FP,
	 "DPCP no nesting allowed")
{
	int od, od2;
	int cpu = 0;

	SYSCALL( sporadic_partitioned(10, 100, 0) );
	SYSCALL( task_mode(LITMUS_RT_TASK) );

	SYSCALL( od  = litmus_open_lock(DPCP_SEM, 0, ".dpcp_locks", &cpu) );
	SYSCALL( od2 = litmus_open_lock(DPCP_SEM, 1, ".dpcp_locks", &cpu) );

	SYSCALL( litmus_lock(od) );
	SYSCALL( litmus_unlock(od) );

	SYSCALL( litmus_lock(od2) );
	SYSCALL( litmus_unlock(od2) );

	SYSCALL( litmus_lock(od) );
	SYSCALL_FAILS(EBUSY, litmus_lock(od2));
	SYSCALL( litmus_unlock(od) );

	SYSCALL( litmus_lock(od2) );
	SYSCALL_FAILS(EBUSY, litmus_lock(od));
	SYSCALL( litmus_unlock(od2) );

	SYSCALL( od_close(od) );
	SYSCALL( od_close(od2) );

	SYSCALL( remove(".dpcp_locks") );
}

TESTCASE(dflp_nesting, P_FP,
	 "DFLP no nesting allowed")
{
	int od, od2;
	int cpu = 0;

	SYSCALL( sporadic_partitioned(10, 100, 0) );
	SYSCALL( task_mode(LITMUS_RT_TASK) );

	SYSCALL( od  = litmus_open_lock(DFLP_SEM, 0, ".dflp_locks", &cpu) );
	SYSCALL( od2 = litmus_open_lock(DFLP_SEM, 1, ".dflp_locks", &cpu) );

	SYSCALL( litmus_lock(od) );
	SYSCALL( litmus_unlock(od) );

	SYSCALL( litmus_lock(od2) );
	SYSCALL( litmus_unlock(od2) );

	SYSCALL( litmus_lock(od) );
	SYSCALL_FAILS(EBUSY, litmus_lock(od2));
	SYSCALL( litmus_unlock(od) );

	SYSCALL( litmus_lock(od2) );
	SYSCALL_FAILS(EBUSY, litmus_lock(od));
	SYSCALL( litmus_unlock(od2) );

	SYSCALL( od_close(od) );
	SYSCALL( od_close(od2) );

	SYSCALL( remove(".dflp_locks") );
}

TESTCASE(lock_fmlp_dflp_no_nesting, P_FP,
	 "DFLP and FMLP nesting not allowed")
{
	int od, od2;
	const char* namespace = ".locks";
	int cpu = 0;

	SYSCALL( sporadic_partitioned(10, 100, 0) );
	SYSCALL( task_mode(LITMUS_RT_TASK) );

	SYSCALL( od  = litmus_open_lock(DFLP_SEM, 0, namespace, &cpu) );
	SYSCALL( od2 = litmus_open_lock(FMLP_SEM, 1, namespace, NULL) );

	SYSCALL( litmus_lock(od) );
	SYSCALL( litmus_unlock(od) );

	SYSCALL( litmus_lock(od2) );
	SYSCALL( litmus_unlock(od2) );

	SYSCALL( litmus_lock(od) );
	SYSCALL_FAILS(EBUSY, litmus_lock(od2));
	SYSCALL( litmus_unlock(od) );

	SYSCALL( litmus_lock(od2) );
	SYSCALL_FAILS(EBUSY, litmus_lock(od));
	SYSCALL( litmus_unlock(od2) );

	SYSCALL( od_close(od) );
	SYSCALL( od_close(od2) );

	SYSCALL( remove(namespace) );
}

TESTCASE(lock_dflp_pcp_no_nesting, P_FP,
	 "PCP and DFLP nesting not allowed")
{
	int od, od2;
	int cpu = 0;
	const char* namespace = ".locks";

	SYSCALL( sporadic_partitioned(10, 100, 0) );
	SYSCALL( task_mode(LITMUS_RT_TASK) );

	SYSCALL( od  = litmus_open_lock(PCP_SEM, 0, namespace, NULL) );
	SYSCALL( od2 = litmus_open_lock(DFLP_SEM, 1, namespace, &cpu) );

	SYSCALL( litmus_lock(od) );
	SYSCALL( litmus_unlock(od) );

	SYSCALL( litmus_lock(od2) );
	SYSCALL( litmus_unlock(od2) );

	SYSCALL( litmus_lock(od) );
	SYSCALL_FAILS(EBUSY, litmus_lock(od2));
	SYSCALL( litmus_unlock(od) );

	SYSCALL( litmus_lock(od2) );
	SYSCALL_FAILS(EBUSY, litmus_lock(od));
	SYSCALL( litmus_unlock(od2) );

	SYSCALL( od_close(od) );
	SYSCALL( od_close(od2) );

	SYSCALL( remove(namespace) );
}
