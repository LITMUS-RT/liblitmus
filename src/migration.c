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

static int read_mapping(int idx, const char* which, cpu_set_t** set, size_t *sz)
{
	/* Max CPUs = 4096 */

	int	ret = -1;
	char buf[4096/4     /* enough chars for hex data (4 CPUs per char) */
	       + 4096/(4*8) /* for commas (separate groups of 8 chars) */
	       + 1] = {0};  /* for \0 */
	char fname[80] = {0};

	char* chunk_str;
	int len, nbits;
	int i;

	/* init vals returned to callee */
	*set = NULL;
	*sz = 0;

	if (num_online_cpus() > 4096)
		goto out;

	/* Read string is in the format of <mask>[,<mask>]*. All <mask>s following
	   a comma are 8 chars (representing a 32-bit mask). The first <mask> may
	   have fewer chars. Bits are MSB to LSB, left to right. */
	snprintf(fname, sizeof(fname), "/proc/litmus/%s/%d", which, idx);
	ret = read_file(fname, &buf, sizeof(buf)-1);
	if (ret <= 0)
		goto out;

	len = strnlen(buf, sizeof(buf));
	nbits = 32*(len/9) + 4*(len%9); /* compute bits, accounting for commas */

	*set = CPU_ALLOC(nbits);
	*sz = CPU_ALLOC_SIZE(nbits);
	CPU_ZERO_S(*sz, *set);

	/* process LSB chunks first (at the end of the str) and move backward */
	chunk_str = buf + len - 9;
	i = 0;
	do
	{
		unsigned long chunk;
		if(chunk_str < buf)
			chunk_str = buf; /* when MSB mask is less than 8 chars */
		chunk = strtoul(chunk_str, NULL, 16);
		while (chunk) {
			int j = ffsl(chunk) - 1;
			int x = i*32 + j;
			CPU_SET_S(x, *sz, *set);
			chunk &= ~(1ul << j);
		}
		chunk_str -= 9;
		i += 1;
	} while(chunk_str >= buf - 8);

	ret = 0;

out:
	return ret;
}

static unsigned long long int cpusettoull(cpu_set_t* bits, size_t sz)
{
	unsigned long long mask = 0;
	int i;

	for (i = 0; i < sizeof(mask)*8; ++i) {
		if (CPU_ISSET_S(i, sz, bits)) {
			mask |= (1ull) << i;
		}
	}

	return mask;
}

int domain_to_cpus(int domain, unsigned long long int* mask)
{
	/* TODO: Support more than 64 CPUs. Instead of using 'ull' for 'mask',
	   consider using gcc's __uint128_t or some struct. */

	cpu_set_t *bits;
	size_t sz;
	int ret;

	/* number of CPUs exceeds what we can pack in ull */
	if (num_online_cpus() > sizeof(unsigned long long int)*8)
		return -1;

	ret = read_mapping(domain, "domains", &bits, &sz);
	if (!ret) {
		*mask = cpusettoull(bits, sz);
		CPU_FREE(bits);
	}

	return ret;
}

int cpu_to_domains(int cpu, unsigned long long int* mask)
{
	/* TODO: Support more than 64 domains. Instead of using 'ull' for 'mask',
	   consider using gcc's __uint128_t or some struct. */

	cpu_set_t *bits;
	size_t sz;
	int ret;

	/* number of CPUs exceeds what we can pack in ull */
	if (num_online_cpus() > sizeof(unsigned long long int)*8)
		return -1;

	ret = read_mapping(cpu, "cpus", &bits, &sz);
	if (!ret) {
		*mask = cpusettoull(bits, sz);
		CPU_FREE(bits);
	}

	return ret;
}

int domain_to_first_cpu(int domain)
{
	cpu_set_t *bits;
	size_t sz;
	int i, n_online;
	int first;

	int ret = read_mapping(domain, "domains", &bits, &sz);

	if (ret)
		return ret;

	n_online = num_online_cpus();
	first = -1; /* assume failure */
	for (i = 0; i < n_online; ++i) {
		if(CPU_ISSET_S(i, sz, bits)) {
			first = i;
			break;
		}
	}
	CPU_FREE(bits);

	return first;
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
	int	ret;
	cpu_set_t *cpu_set;
	size_t sz;

	ret = read_mapping(domain, "domains", &cpu_set, &sz);
	if (ret != 0)
		return ret;

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
