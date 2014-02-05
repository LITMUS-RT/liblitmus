/**
 * @file migration.h
 * Functions to migrate tasks to different CPUs, partitions, clusters...
 */

typedef int pid_t;

/**
 * obtain the PID of a thread (TID)
 * @return The PID of a thread
 */
pid_t gettid();

/**
 * Migrate and assign a task to a given CPU
 * @param tid Process ID for migrated task, 0 for current task
 * @param target_cpu ID for CPU to migrate to
 * @pre tid is not yet in real-time mode (it's a best effort task)
 * @return 0 if successful
 */
int be_migrate_thread_to_cpu(pid_t tid, int target_cpu);

/**
 * Migrate current task to a given cluster
 * @param tid Process ID for migrated task, 0 for current task
 * @param domain Cluster ID to migrate the task to
 * @pre tid is not yet in real-time mode (it's a best effort task)
 * @return 0 if successful
 */
int be_migrate_thread_to_cluster(pid_t tid, int domain);

/**
 * Migrate current task to a given CPU
 * @param target_cpu ID for CPU to migrate to
 * @pre The current task is not yet in real-time mode (it's a best-effort task)
 * @return 0 if successful
 */
int be_migrate_to_cpu(int target_cpu);

/**
 * Migrate current task to a given cluster
 * @param cluster Cluster ID to migrate the task to
 * @param cluster_sz Size of the cluster to migrate to (unused)
 * @pre The current task is not yet in real-time mode (it's a best-effort task)
 * @return 0 if successful
 *
 * \deprecated{Use be_migrate_to_domain() instead in new code.}
 */
int be_migrate_to_cluster(int cluster, int cluster_sz);

/**
 * Migrate current task to a given scheduling domain (i.e., cluster or
 * partition).
 * @param domain The cluster/partition to migrate to.
 * @pre The current task is not yet in real-time mode (it's a best-effort task).
 * @return 0 if successful
 */
int be_migrate_to_domain(int domain);

/**
 * Return the number of CPUs currently online
 * @return The number of online CPUs
 */
int num_online_cpus();

/**
 * @todo Document!
 */
int release_master();
int domain_to_cpus(int domain, unsigned long long int* mask);
int cpu_to_domains(int cpu, unsigned long long int* mask);

int domain_to_first_cpu(int domain);
