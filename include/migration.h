
typedef int pid_t;

/* obtain the PID of a thread */
pid_t gettid();

/* Assign a task to a cpu/partition/cluster.
 * PRECOND: tid is not yet in real-time mode (it's a best effort task).
 * Set tid == 0 to migrate the caller */
int be_migrate_thread_to_cpu(pid_t tid, int target_cpu);
int be_migrate_thread_to_partition(pid_t tid, int partition);
/* If using release master, set cluster_sz to size of largest cluster. tid
 * will not be scheduled on release master. */
int be_migrate_thread_to_cluster(pid_t tid, int cluster, int cluster_sz);

/* set ignore_rm == 1 to include release master in tid's cpu affinity */
int __be_migrate_thread_to_cluster(pid_t tid, int cluster, int cluster_sz, int ignore_rm);

int be_migrate_to_cpu(int target_cpu);
int be_migrate_to_partition(int partition);
int be_migrate_to_cluster(int cluster, int cluster_sz);

int num_online_cpus();
int release_master();
