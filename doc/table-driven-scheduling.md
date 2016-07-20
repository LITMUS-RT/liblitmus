# HOWTO: Table-Driven Scheduling 

As of version 2016.1, LITMUS^RT supports *table-driven scheduling* (aka *time partitioning*, *static scheduling*, or *time-triggered scheduling*).

In a nutshell:

- The support for table-driven scheduling is realized as a *reservation type* in the `P-RES` plugin.

- Each such *table-driven reservation*  is a *cyclically repeating* sequence of arbitrarily many, arbitrarily sized *scheduling slots*. 

- As with all reservation types supported by the `P-RES` plugin, multiple processes can be assigned to the same table-driven reservations. 

- Processes that are assigned to a table-driven reservation are eligible to be scheduled only during the reservation’s scheduling slots. 

- On multiprocessors, table-driven reservations are *partitioned*:  each reservation is restricted to have scheduling slots on only one processor, but there can be (and usually are) many different table-driven reservations on each core.

- Table-driven reservations can be freely combined with other reservation types supported by the `P-RES` plugin.

## Concepts and Design

The basic idea behind table-driven scheduling is to define a *static scheduling table* that periodically repeats in time. For each point in time, the static scheduling table determines which entity each processor should be executing (if any), and whether any processors are free to execute a background workload (or to idle).

### Table-Driven Reservations in LITMUS^RT

In LITMUS^RT^, the scheduling table does not directly reference specific tasks or threads. Instead, the static scheduling table references reservations (i.e., process *containers*). Within each reservation, contained processes are dispatched dynamically. This approach is conceptually very similar to *time partitions* as defined by [ARINC 653](https://en.wikipedia.org/wiki/ARINC_653).

From a user’s point of view, this split makes it much easier to work with scheduling tables, as the to-be-scheduled real-time processes do not yet have to exist when the scheduling table is installed (i.e., during system initialization).  Furthermore, the scheduling table does not have to reference ephemeral data such as PIDs, which makes it possible to reuse a static table setup across many runs. Processes can be launched and associated with table-driven reservations (i.e., placed in time partitions) at any time *after* the scheduling table has been installed. 

The reservation-based approach in LITMUS^RT results in a simple two-level scheduling scheme: whenever a table-driven reservation is selected for service (i.e., during one of its slots), it selects and dispatches one of its associated processes (if any), or yields the processor to background tasks (if the reservation is “empty” or if none of the associated processes is ready).

If there are multiple ready processes in a reservation, the current reservation implementation defaults to a simple round-robin scheme among all ready client processes. However, support for priority-based arbitration among client processes would be trivial to add, if needed.

### Key Parameters

The schedule created by a table-driven reservation is determined by two principal parameters:

1. the **major cycle** *M*, which is the period of the scheduling table (i.e., at runtime, the schedule repeats every *M* time units), and
2. a sequence of *non-overlapping* **scheduling intervals** (or **slots**), where each such scheduling slot *[S, E)* is defined by a
	- *start offset* *S*  in the range [0, *M*) and an
	- *end offset* *E* in the range (*S*, *M*).

For example, suppose we are given a table-driven reservation with

- a major cycle of *M=250ms* and 
- scheduling slots *[50ms, 60ms)*, *[100ms, 125ms)*, *[200ms, 205ms)*.

At runtime, processes of this reservation are eligible to execute during all intervals 

- *[k ∙ M + 50, k ∙ M + 60)*, 
- *[k ∙ M + 100, k ∙ M + 125)*, and
- *[k ∙ M + 200, k ∙ M + 205),* 

for *k = 0, 1, 2, 3, …*, relative to some reference time zero (e.g., usually the boot time of the system).

On a given processor, all scheduling slots must be disjoint. That is, in a correct scheduling table, at any point in time *t* and for each processor, there is at most one scheduling interval that contains *t*.


## Creating a Table-Driven Reservation

Since time partitioning is realized with reservations in LITMUS^RT^, a scheduling table is comprised of one or more table-driven reservations. Each such table-driven reservation must be  explicitly created prior to use with the `resctl` utility.  (For an introduction to `resctl`, see the guide [HOWTO: Working with Reservations](howto-use-resctl.md)).

The general form of the command is:

	resctl -n RID -c CPU -t table-driven -m MAJOR-CYCLE SLOT1 SLOT2 SLOT3… SLOT_N

where RID denotes the reservation ID, and CPU denotes the processor on which the reservation is created. Each slot is simply a semi-open interval of the form `[START_TIME, END_TIME)`. All times are specified in (possibly fractional) milliseconds.

For example, the following command instructs `resctl` to allocate a new table-driven reservation (`-t table-driven`)  with ID 123 (`-n 123`) on processor 2 (`-c 2`), with a major cycle of 250ms (`-m 250`) and scheduling slots as mentioned in the preceding example.

	resctl -n 123 -c 2 -t table-driven -m 250 '[50, 60)' '[100, 125)' '[200, 205)'

When using `bash` or a similar shell, the slot specifications must be quoted to prevent the shell from interpreting the closing parentheses as shell syntax. 

If the command succeeds in creating the reservation, there is no output. Otherwise, an error message is shown.

## Installing a Scheduling Table

To define an entire scheduling table comprised of many reservations on multiple cores, execute an appropriate `resctl` command for each reservation in the table. 

A typical approach is to store the system’s scheduling table as a simple shell script that enables the `P-RES` plugin and then runs the required `resctl` invocations. For example, a table-setup script such as the below example can be run as an init script during system boot, or when launching an experiment.

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

While reservation IDs are arbitrary, it is often convenient to use a consistent reservation ID scheme such as *”processor ID” × 1000 + index*.

**WARNING**: While `resctl` checks that each reservation’s slots do not overlap with other slots of the *same* reservation, it is possible to (accidentally) specify *multiple* reservations with overlapping slots. A table with such overlapping slots is erroneous  and results in undefined behavior, but not automatically detected. *It is the responsibility of the user to install a scheduling table in which all slots are non-overlapping.*

Also note that it is possible to specify  a *different* major cycle for each table-driven reservation. If used carefully, this  offers some convenient flexibility. For instance, in some cases, it can be used to efficiently store tables that would otherwise consume large amounts of memory (e.g., if the workload’s hyper-period is large). However, this is an advanced feature and requires some careful consideration to be used correctly (i.e., without accidentally creating overlapping slots). For simplicity, it’s best to use a uniform major cycle parameter for all table-driven reservations.

## Attaching Tasks to Table-Driven Reservations

Once all table-driven reservations have been set up, processes can be attached to them just as it is done with any other reservation type. 

In short, use the `resctl` utility with the *attach* option (`-a`) to associate an already running process, and use the `rt_launch` utility with the *reservation* option (`-r`) to launch a new task in a reservation.

For example, to insert the user’s shell into the reservation with ID 123 on processor 2, run the following command:

	resctl -a $$ -r 123 -c 2

All processes subsequently launched from the shell (i.e., all forked children) automatically become clients of the same reservation.

See the guide [HOWTO: Working with Reservations](howto-use-resctl.md) for further details on attaching real-time tasks to reservations.

## Coordinating Task Activations

It is often useful to ensure that a periodic task that is assigned to a table-driven reservation always wakes up at the beginning of each scheduling slot. While LITMUS^RT does not provide a custom interface for this purpose, it can be accomplished with stock Linux APIs.

Specifically, appropriate task activations can be programmed with the [`clock_nanosleep()`](http://linux.die.net/man/2/clock_nanosleep) API. The reference *time zero* of the repeating table schedule is literally time zero on the timeline of `CLOCK_MONOTONIC`.

LITMUS^RT provides two simple wrapper APIs to work with `CLOCK_MONOTONIC`:

- `litmus_clock()`, which provides the current time  according to `CLOCK_MONOTONIC` as a 64-bit value of type `lt_t` (the LITMUS^RT time type); and
- `lt_sleep_until()`, which suspends the calling process until the provided point in time.

Using these two APIs, it is easy to coordinate for a process to wake up at the beginning of a given scheduling slot. 

For example, suppose we want a task to wake up at the beginning of every instance of a slot *[50ms, 60ms)* of a table-driven reservation with major cycle *M=250ms*. This can be accomplished with the following code:

	lt_t cycle_length, slot_offset, now, next_cycle_start;
	
	/* major cycle length in nanoseconds */
	cycle_length = ms2ns(250); /* ms2ns() is from litmus.h */
	
	/* slot offset in nanoseconds */
	slot_offset = ms2ns(50);
	
	/* main task loop: one iteration == one job */
	while (1) {
		/* get current time in nanoseconds */
		now = litmus_clock();
	
		/* round up to start time of next major cycle */
		next_cycle_start = ((now / cycle_length) + 1) * cycle_length;
	
		/* sleep until beginning of next slot */
		lt_sleep_until(next_cycle_start + slot_offset);
	
		/* when the task resumes, it's beginning of a slot */
		
		 /* application logic */
		execute_the_job();
	
		/* job is complete: loop and sleep until next slot comes around */	
	}

This example code works if the job never overruns its provided slot length. If jobs can overrun, the computation of the next start time needs to be made more robust by accounting for potential overruns.


## Combining Reservation Types

It is possible to combine table-driven scheduling with other reservation types by assigning appropriate reservation priorities. There are two possible use cases:

1. event-driven reservations that serve as lower-priority *background* reservations, which are scheduled only when the table is idle, and
2. higher-priority, *throttled* reservations that can preempt any tasks in   table-driven reservations.

The first use case is the normal scenario (the table takes precedence at all times), but the second use case (the table can be *briefly* overruled) can also make sense when there is a need support low-latency applications. We briefly sketch both setups.

### Lower-Priority Background Reservations

By default, table-driven reservations are given the highest-possible priority, which numerically is priority 1. This means that lower-priority, event-driven reservations can be trivially added simply by giving them any priority other than priority 1 (with `resctl`’s `-q` option).

For example, to add a sporadic polling reservation on processor 2 with ID 9999,  priority 99, a budget of 100ms, and a period of 500ms, execute the following command:

	resctl -n 9999 -c 2  -t polling-sporadic -q 99 -b 100 -p 500

EDF priorities are even lower, so it is possible to combine table-driven reservations with EDF-scheduled background reservations. For instance, the command (which omits the `-q` option)

	resctl -n 8888 -c 2  -t polling-sporadic -b 100 -p 500

adds an EDF-scheduled background reservation.


### Higher-Priority Foreground Reservations

It is possible to define fixed-priority reservations that can preempt table-driven reservations at any and all times during table-driven scheduling slots. The primary use case for such a setup is to ensure timely processor service for *latency-sensitive* tasks (such as certain interrupt threads) that do not consume a lot of processor bandwidth, but that need to be scheduled *quickly* when new data arrives. This, of course, makes sense only if the higher-priority reservations are tightly rate-limited to ensure that table allocations are not completely starved. 

To create a sporadic polling reservation that can preempt table-driven reservations, do the following.

- When creating table-driven reservations, explicitly specify a  priority lower than 1 (i.e., numerically larger than 1) by providing the `-q` switch.

	Example: `resctl -q 20 -n 123 -c 2 -t table-driven -m 250 '[50, 60)' '[100, 125)' '[200, 205)'`

-  When creating the sporadic polling reservation, explicitly specify a priority higher than (i.e., numerically smaller than) the one given to table-driven reservations.

	Example: `resctl -q 19 -n 9999 -c 2  -t polling-sporadic -b 1 -p 500`

When preempted by a higher-priority reservation, the budget of table-driven reservations is *not* shifted: any cycles that cannot be used during a scheduling slot due to higher-priority interference are simply lost. 

Note that it is possible to give each table-driven reservation a different priority. This allows for precise control over *which* table-driven reservations may be preempted by event-driven reservations.  

In fact, it is even possible to install *multiple*, *intentionally overlapping* table-driven reservations at different priority levels. This could potentially be used to realize a notion of *mixed-criticality* tables such as those described by [Baruah and Fohler (2011)](http://ieeexplore.ieee.org/xpls/abs_all.jsp?arnumber=6121421). A detailed exploration of this approach is subject to future work.
