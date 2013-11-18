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
 * Migrate and assign a task to a given partition
 * @param tid Process ID for migrated task, 0 for current task
 * @param partition Partition ID to migrate the task to
 * @pre tid is not yet in real-time mode (it's a best effort task)
 * @return 0 if successful
 */
int be_migrate_thread_to_partition(pid_t tid, int partition);
/**
 * Migrate current task to a given cluster
 * @param tid Process ID for migrated task, 0 for current task
 * @param cluster Cluster ID to migrate the task to
 * @param cluster_sz Size of the cluster to migrate to
 * @pre tid is not yet in real-time mode (it's a best effort task)
 * @return 0 if successful
 *
 * If using release master, set cluster_sz to size of largest cluster. tid
 * will not be scheduled on release master
 */
int be_migrate_thread_to_cluster(pid_t tid, int cluster, int cluster_sz);

/**
 * @private
 * set ignore_rm == 1 to include release master in tid's cpu affinity
 */
int __be_migrate_thread_to_cluster(pid_t tid, int cluster, int cluster_sz, int ignore_rm);

/**
 * Migrate current task to a given CPU
 * @param target_cpu ID for CPU to migrate to
 * @pre tid is not yet in real-time mode (it's a best effort task)
 * @return 0 if successful
 */
int be_migrate_to_cpu(int target_cpu);
/**
 * Migrate current task to a given partition
 * @param partition Partition ID to migrate the task to
 * @pre tid is not yet in real-time mode (it's a best effort task)
 * @return 0 if successful
 */
int be_migrate_to_partition(int partition);
/**
 * Migrate current task to a given cluster
 * @param cluster Cluster ID to migrate the task to
 * @param cluster_sz Size of the cluster to migrate to
 * @pre tid is not yet in real-time mode (it's a best effort task)
 * @return 0 if successful
 */
int be_migrate_to_cluster(int cluster, int cluster_sz);

/**
 * Return the number of CPUs currently online
 * @return The number of online CPUs
 */
int num_online_cpus();
/**
 * @todo Document!
 */
int release_master();
