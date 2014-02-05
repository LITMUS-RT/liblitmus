#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sched.h> /* for cpu sets */
#include <unistd.h>

#include "migration.h"

extern ssize_t read_file(const char* fname, void* buf, size_t maxlen);

int release_master()
{
	static const char NO_CPU[] = "NO_CPU";
	char buf[7] = {0}; /* up to 999999 CPUs */
	int master = -1;

	int ret = read_file("/proc/litmus/release_master", &buf, sizeof(buf)-1);

	if ((ret > 0) && (strncmp(buf, NO_CPU, sizeof(NO_CPU)-1) != 0))
		master = atoi(buf);

	return master;
}

int num_online_cpus()
{
	return sysconf(_SC_NPROCESSORS_ONLN);
}

static int read_mapping(int idx, const char* which, unsigned long long int* mask)
{
	int	ret = -1;
	char buf[129] = {0};
	char fname[80] = {0};

	if (num_online_cpus() > 64) {
		/* XXX: Support more than 64 CPUs.
		 * User can still set appropriate values directly. */
		goto out;
	}

	snprintf(fname, sizeof(fname), "/proc/litmus/%s/%d", which, idx);

	ret = read_file(fname, &buf, sizeof(buf)-1);
	if (ret <= 0)
		goto out;

	*mask = strtoull(buf, NULL, 16);
	ret = 0;

out:
	return ret;
}

int domain_to_cpus(int domain, unsigned long long int* mask)
{
	return read_mapping(domain, "domains", mask);
}

int cpu_to_domains(int cpu, unsigned long long int* mask)
{
	return read_mapping(cpu, "cpus", mask);
}

int domain_to_first_cpu(int domain)
{
	unsigned long long int mask;
	int ret = domain_to_cpus(domain, &mask);
	if(ret == 0)
		return (ffsll(mask)-1);
	return ret;
}

int be_migrate_thread_to_cpu(pid_t tid, int target_cpu)
{
	cpu_set_t *cpu_set;
	size_t sz;
	int num_cpus;
	int ret;

	/* TODO: Error check to make sure that tid is not a real-time task. */

	if (target_cpu < 0)
		return -1;

	num_cpus = num_online_cpus();
	if (num_cpus == -1)
		return -1;

	if (target_cpu >= num_cpus)
		return -1;

	cpu_set = CPU_ALLOC(num_cpus);
	sz = CPU_ALLOC_SIZE(num_cpus);
	CPU_ZERO_S(sz, cpu_set);
	CPU_SET_S(target_cpu, sz, cpu_set);

	/* apply to caller */
	if (tid == 0)
		tid = gettid();

	ret = sched_setaffinity(tid, sz, cpu_set);

	CPU_FREE(cpu_set);

	return ret;
}

int be_migrate_thread_to_domain(pid_t tid, int domain)
{
	int	ret, num_cpus;
	cpu_set_t *cpu_set;
	size_t sz;
	unsigned long long int mask;

	ret = domain_to_cpus(domain, &mask);
	if (ret != 0)
		return ret;

	num_cpus = num_online_cpus();
	if (num_cpus == -1)
		return -1;

	cpu_set = CPU_ALLOC(num_cpus);
	sz = CPU_ALLOC_SIZE(num_cpus);
	CPU_ZERO_S(sz, cpu_set);

	while(mask) {
		int idx = ffsll(mask) - 1;
		CPU_SET_S(idx, sz, cpu_set);
		mask &= ~(1ull<<idx);
	}

	/* apply to caller */
	if (tid == 0)
		tid = gettid();

	ret = sched_setaffinity(tid, sz, cpu_set);

	CPU_FREE(cpu_set);

	return ret;
}

int be_migrate_to_cpu(int target_cpu)
{
	return be_migrate_thread_to_cpu(0, target_cpu);
}

int be_migrate_to_domain(int domain)
{
	return be_migrate_thread_to_domain(0, domain);
}


/* deprecated functions. */

int be_migrate_to_cluster(int cluster, int cluster_size)
{
	return be_migrate_to_domain(cluster);
}

int cluster_to_first_cpu(int cluster, int cluster_size)
{
	return domain_to_first_cpu(cluster);
}

int partition_to_cpu(int partition)
{
	return domain_to_first_cpu(partition);
}
