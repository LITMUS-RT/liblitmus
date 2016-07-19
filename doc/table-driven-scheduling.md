# HOWTO: Table-Driven Scheduling 

As of version 2016.1, LITMUS^RT supports *table-driven scheduling* (aka *time partitioning*, *static scheduling*, or *time-triggered scheduling*).

In a nutshell:

- The support for time-driven scheduling is realized as a *reservation type* in the `P-RES` plugin.

- Each such *table-driven reservation*  is a *cyclically repeating* sequence of arbitrarily many, arbitrarily sized *scheduling slots*. 

- As with all reservation types supported by the `P-RES` plugin, multiple processes can be assigned to the same table-driven reservations. 

- Processes that are assigned to a table-driven reservation are eligible to be scheduled only during the reservation’s scheduling slots. 

- On multiprocessors, table-driven reservations are *partitioned*:  each reservation is restricted to have scheduling slots on only one processor, but there can be (and usually are) many different table-driven reservations on each core.

- Time-driven reservations can be freely combined with other reservation types supported by the `P-RES` plugin.

## Concepts and Design

The basic idea behind table-driven scheduling is to define a *static scheduling table* that periodically repeats in time. For each point in time, the static scheduling table determines which entity each processor should be executing (if any), and whether any processors are free to execute a background workload (or idle).

### Table-Driven Reservations in LITMUS^RT

In LITMUS^RT, the scheduling table does not directly reference specific tasks or threads. Instead, the static scheduling table references reservations (i.e., process *containers*). Within each reservation, contained processes are dispatched dynamically. This approach is conceptually very similar to time partitions as defined by ARINC 653.

From a user’s point of view, this split makes it much easier to work with scheduling tables, as the to-be-scheduled real-time processes do not yet have to exist when the scheduling table is installed (i.e., during system initialization).  Furthermore, the scheduling table does not have to reference ephemeral PIDs, which makes it possible to reuse a fixed table setup across many runs. Processes can be launched and associated with table-driven reservations (i.e., placed in time partitions) at any time *after* the scheduling table was installed. 

The reservation-based approach in LITMUS^RT results in a simple two-level scheduling scheme: whenever a table-driven reservation is selected for service (i.e., during one of its slots), it selects and dispatches one of its associated processes (if any), or yields the processor to background tasks (if the reservation is “empty” or if none of the associated processes are ready).

If there are multiple ready processes in a reservation, the current table-driven reservation implementation defaults to a simple round-robin scheme among all ready client processes. However, support for priority-arbitration among client processes would be trivial to add, if needed.

### Key Parameters

The schedule created by a table-driven reservation is determined by two principal parameters:

1. the **major cycle** *M*, which is the period of the scheduling table, and
2. a sequence of *non-overlapping* **scheduling intervals** (or **slots**), where each such scheduling slot *[S, E)* is defined by a
	- *start offset* *S*  in the range [0, *M*) and a
	- *end offset* *E* in the range (*S*, *M*).

For example, suppose we are given a table-driven reservation with

- a major cycle of *M=250ms* and 
- scheduling slots *[50ms, 60ms)*, *[100ms, 125ms)*, *[200ms, 205ms)*.

At runtime, processes of this reservation are eligible to execute during all intervals, for *k = 0, 1, 2, 3, …*, 

- *[k ∙ M + 50, k ∙ M + 60)*, 
- *[k ∙ M + 100, k ∙ M + 125)*, and
- *[k ∙ M + 200, k ∙ M + 205),* 

relative to some reference time zero (i.e., usually the boot time of the system).

On a given processor, all scheduling slots must be disjoint. That is, in a correct scheduling table, at any point in time *t* and on each processor, there is at most one scheduling interval that contains *t*.


## Creating Table-Driven Reservations

Since time partitioning is realized with reservations in LITMUS^RT, a scheduling table is comprised of one or more table-driven reservations. Each such table-driven reservation must be  explicitly created prior to use with the `resctl` utility.  (For an introduction to `resctl`, see the guide [HOWTO: Working with Reservations](howto-use-resctl.md)).

The general form of the command is:

	resctl -n RID -c CPU -t table-driven -m MAJOR-CYCLE SLOT1 SLOT2 SLOT3… SLOT_N

where RID denotes the reservation ID, and CPU denotes the processor on which the reservation is created. Each slot is simply a semi-open interval of the form `[START_TIME, END_TIME)`. 

For example, the following command instructs `resctl` to allocate a new table-driven reservation (`-t table-driven`)  with ID 123 (`-n 123`) on processor 2 (`-c 2`), with a major cycle of 250ms (`-m 250`) and scheduling slots as mentioned in the preceding example.

	resctl -n 123 -c 2 -t table-driven -m 250 '[50, 60)' '[100, 125)' '[200, 205)'

When using the `bash` shell, the slot specifications must be quoted to prevent the shell from interpreting the closing parentheses as shell syntax. 

If the command succeeds in creating the reservation, there is no output. Otherwise, an error message is shown.

To define an entire scheduling table comprised of many reservations on multiple cores, execute an appropriate `resctl` command for each reservation in the table. A typical approach is to store the system’s scheduling table as a simple shell script that enables the `P-RES` plugin and then runs the required `resctl` invocations. For example, a table-setup script such as the below example can be run as an init script during system boot, or when launching an experiment.

	#!/bin/sh

	# Enable plugin
	setsched P-RES
	
	# Major cycle of the static schedule, in milliseconds.
	MAJOR_CYCLE=…
	
	# Install table for core 0
	resctl -c 0 -n 1 -t table-driven -m $MAJOR_CYCLE …
	resctl -c 0 -n 2 -t table-driven -m $MAJOR_CYCLE …
	resctl -c 0 -n 3 -t table-driven -m $MAJOR_CYCLE …
	…
	
	# Install table for core 1
	resctl -c 1 -n 1001 -t table-driven -m $MAJOR_CYCLE …
	resctl -c 1 -n 1002 -t table-driven -m $MAJOR_CYCLE …
	resctl -c 1 -n 1003 -t table-driven -m $MAJOR_CYCLE …
	…
	
	# Install table for core 2
	resctl -c 2 -n 2001 -t table-driven -m $MAJOR_CYCLE …
	resctl -c 2 -n 2002 -t table-driven -m $MAJOR_CYCLE …
	resctl -c 2 -n 2003 -t table-driven -m $MAJOR_CYCLE …
	…

(Reservation IDs are arbitrary. As a simple convention, it is convenient to use a consistent reservation ID scheme such as *processor ID ∙ 1000 + index*.)

**WARNING**: While `resctl` checks that each reservation’s slots do not overlap with other slots of the *same* reservation, it is possible to (accidentally) specify *multiple* reservations with overlapping slots. A table with such overlapping slots is erroneous and results in undefined behavior. *It is the responsibility of the user to install a scheduling table in which all slots are non-overlapping.*

Also note that it is possible to specify  a *different* major cycle for each table-driven reservation. If used carefully, this  offers some convenient flexibility and can for instance be used to more efficiently store large tables that would otherwise consume large amounts of memory. However, this is an advanced feature and requires some careful consideration to be used correctly (i.e., without accidentally creating overlapping slots). For simplicity, it’s best to use a uniform major cycle parameter for all table-driven reservations.

## Attaching Tasks to Table-Driven Reservations

Once all table-driven reservations have been set up, processes can be attached to them just as it is done with any other reservation type. 

In short, use the `resctl` utility with the *attach* option (`-a`) to associate an already running process, and use the `rt_launch` utility with the *reservation* option (`-r`) to launch a new task in a reservation.

For example, to insert the user’s shell into the reservation with ID 123 on processor 2, run the following command:

	resctl -a $$ -r 123 -c 2

See the guide [HOWTO: Working with Reservations](howto-use-resctl.md) for further details.

## Coordinating Task Activations

It can be useful to ensure that a periodic task that is assigned to a table-driven reservation always wakes up at the beginning of each scheduling slot. While LITMUS^RT does not provide a custom interface for this purpose, it can be accomplished with stock Linux APIs. Specifically, appropriate task activations can be programmed with the `clock_nanosleep()` API. The reference *time zero* of the repeating table schedule is literally time zero on the timeline of `CLOCK_MONOTONIC`.


## Combining Reservation Types

It is possible to combine table-driven scheduling with other reservation types by assigning appropriate reservation priorities. There are two possible use cases:

1. event-driven reservations as lower-priority background reservations that are scheduled when the table is idle, and
2. higher-priority, *rate-throttled* reservations that can preempt the tasks in the table-driven reservations.

The first use case is the normal case (the table takes precedence at all times), but the second use case (the table can be *briefly* overruled) can also make sense to support low-latency applications. We briefly sketch both setups.

### Lower-Priority Background Reservations

By default, table-driven reservations are given the highest-possible priority, which numerically is priority 1. This means that lower-priority event-driven reservations can be trivially added simply by giving them a lower priority (with `resctl`’s `-q` option).

For example, to add a sporadic polling reservation on processor 2 with ID 9999,  priority 99, a budget of 100ms, and a period of 500ms, execute the following command:

	resctl -n 9999 -c 2  -t polling-sporadic -q 99 -b 100 -p 500

EDF priorities are even lower, so it is possible to combine table-driven reservations with EDF-scheduled background reservations. For instance, the command

	resctl -n 8888 -c 2  -t polling-sporadic -b 100 -p 500

adds an EDF-scheduled background reservation.


### Higher-Priority Foreground Reservations

It is possible to define fixed-priority reservations that can preempt table-driven reservations at any and all times during table-driven scheduling slots. The primary use case for such a setup is to ensure timely processor service for *latency-sensitive* tasks (such as interrupt threads) that do not consume a lot of processor bandwidth, but that need to be scheduled *quickly* when new data arrives. This, of course, makes sense only if the higher-priority reservations are tightly limited to ensure that table allocations are not completely starved. 

To create a sporadic polling reservation that can preempt table-driven reservations, do the following.

- When creating table-driven reservations, explicitly specify a  priority lower than 1 by providing the `-q` switch.

	Example: `resctl -q 20 -n 123 -c 2 -t table-driven -m 250 '[50, 60)' '[100, 125)' '[200, 205)'`

-  When creating the sporadic polling reservation, explicitly specify a priority higher than the one given to table-driven reservations.

	Example: `resctl -q 19 -n 9999 -c 2  -t polling-sporadic -b 1 -p 500`

When preempted by a higher-priority reservation, the budget of table-driven reservations is *not* shifted: any cycles that cannot be used during a scheduling slot due to higher-priority interference are simply lost. 

Note that it is possible to give each table-driven reservation a different priority. This allows for precise control over *which* table-driven reservations may be preempted by event-driven reservations.  