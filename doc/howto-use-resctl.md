# HOWTO: Working with Reservations

Recent versions of LITMUS^RT support proper **reservation-based scheduling**, and in particular the *partitioned reservation* (`P-RES`) plugin. This guide explains how to work with reservations in LITMUS^RT.

## The P-RES Plugin

The `P-RES` plugin supports *partitioned* uniprocessor reservations.

In the current version, the plugin does not support the LITMUS^RT synchronization API. However, there exists an implementation of the [MC-IPC](https://www.mpi-sws.org/~bbb/papers/pdf/rtss14b.pdf), a synchronous IPC protocol with strong temporal isolation properties,  on top of the `P-RES` plugin in a separate development branch.

In contrast to older LITMUS^RT plugins, reservations are first-class entities in `P-RES`  that exist independently of tasks. In particular, they must be created *before* any task can be launched, and they continue to exist even after all tasks have terminated. Multiple tasks or threads can be assigned to the same reservation. If a task forks, both the parent and the child remain in the same partition.


## Partitioned Reservations

To create new reservations, use the tool `resctl`. The tool has an online help message that can be triggered with `resctl -h`, which explains all options. In the following, we explain how to create partitioned, per-processor reservations.

### Reservation Types

The current version of the `P-RES` plugin supports three reservation types:

```
polling-periodic (PP)
polling-sporadic (PS)
table-driven (TD)
```

Additional common reservations types (e.g., CBS, sporadic servers, etc.) have been developed in a separate branch and will be released in a later version.

The most simple reservation type is the polling reservation, which comes in two flavors:  classic *periodic polling reservations* (PP), and more flexible *sporadic polling reservations* (SP). The latter is ideally suited for encapsulating sporadic and periodic real-time tasks, whereas the former is useful primarily if there is a need for fixed, known replenishment times.

In the following examples, we use sporadic polling reservations.

### Creating a reservation

Each reservation is identified by a **reservation ID (RID)**, which is simply a non-negative number (like a PID). However, there are two important differences between PIDs and RIDs:

1. In contrast to PIDs, RIDs are not automatically assigned. Rather, the desired RID must be specified when the reservation is created.

2. The same RID may be used on multiple processors (i.e., RIDs are not unique systemwide). Hence it is important to specify the processor both when creating reservations and when attaching processes to reservations.

Use the following invocation to create a new sporadic polling reservation with RID 123 on core 0:

```
resctl -n 123 -t polling-sporadic -c 0
```

Unless there is an error, no output is generated.

The above command created a polling reservation with a default budget of 10ms and a replenishment period of 100ms.

To specify other periods and/or budgets, use the `-p` and `-b` options, respectively.

For instance, use the following invocation to create a sporadic polling reservation with RID 1000 on core 1 with a budget of 25ms and a replenishment period of 50ms:

```
resctl -n 1000 -t polling-sporadic -c 1 -b 25 -p 50
```

### Programmatically starting a task in a reservation

To attach a task to a previously created reservation, simply use the RID as the task’s assigned processor (i.e., the RID identifies a *virtual processor*).

For example, to associate the current thread with RID 1000 on core 1 when setting up the thread’s real-time parameters, set the `cpu` field of the `struct rt_task` structure to 1000  prior to calling `set_rt_task_param()`.

Since the same RID may be used on multiple cores, make sure to migrate to the target core  (with `be_migrate_to_domain()`) before calling `task_mode(LITMUS_RT_TASK)`.

Example:

```C
int core = 1;   // the core the reservation is on
int rid = 1000; // the RID to attach to
struct rt_task param;

init_rt_task_param(&param);
// set up exec_cost, period, etc. as usual
param.exec_cost = …
param.period = …
// attach to the reservation
param.cpu = rid;

// upload parameters to kernel
set_rt_task_param(gettid(), &param);

// migrate thread to appropriate core
be_migrate_to_domain(core);

// transition into real-time mode
task_mode(LITMUS_RT_TASK);
```

Error checking has been omitted in this example to avoid clutter. However, each of these calls can fail and the return code must be checked.


### Launching `rtspin` in a reservation

The `rtspin` test application can be assigned to a pre-existing reservation with the `-r` option.

For example, to assign an `rtspin` process that runs for three seconds with period 100 and execution time 10 to reservation 1000 on core 1, launch `rtspin` like this:

```
 rtspin -p 1 -r 1000 10 100 3
```

### Launching any task in a reservation

The same options (`-p` and `-r`) are also understood by the `rt_launch` utility.

For example, to launch a `find` process in reservation 1000 on core 1, use the following command:

```
rt_launch -p 1 -r 1000 find /
```

While `rt_launch` usually requires a budget and period to be specified when used with a process-based scheduler plugin, this is not required when launching a process inside a reservation: since only the parameters of the reservation are relevant for scheduling purposes, the per-process parameters are simply omitted. 

### Attaching an already running task to a reservation

It is also possible to assign an already running, non-real-time task or thread to a reservation. This can be accomplished with the `-a` (attach) option of  `resctl`.

For example, to move the current shell into reservation 1000 on core 1, execute the following command:

```
resctl -a $$ -r 1000 -c 1
```

### Deleting a reservation

There is currently no mechanism to delete individual reservations. Once created, a reservation persists until the scheduler plugin is switched.

To delete *all* reservations, simply switch the active scheduler plugin to Linux and back again.

```
CUR_PLUGIN=`showsched`
setsched Linux
setsched $CUR_PLUGIN
```

## Listing existing reservations

There is currently no way to obtain a list of already created reservations from user space.

(Ideally, this information would be exported via `/sys` or `/proc`, or even better by integrating with Linux's `cgroup` subsystem. Patches welcome.)


