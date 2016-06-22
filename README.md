LITMUS^RT User-Space Library: liblitmus
=======================================

Copyright (c) 2007-2016 The LITMUS^RT Project & Contributors
http://www.litmus-rt.org

Released as open source under the terms of the GNU General Public License
(GPL2).

Overview
--------
This library and the included tools provide the user-space interface to
LITMUS^RT. Real-time tasks should link against this library. The header
file `litmus.h` contains all necessary system calls and definitions to
interact with the kernel services provided for real-time tasks.

Documentation
-------------

For a gentle introduction to LITMUS^RT and its userspace tools, we
recommend to check out the materials that were prepared for the
LITMUS^RT tutorial presented at TuToR'16 @ CPSWeek'16.

- [LITMUS^RT tutorial at TuToR'16 @ CPSWeek'16](http:/www.litmus-rt.org/tutor16)

In particular, check out the online manual prepared for TuToR'16:

- [A Tour of LITMUS^RT](http://www.litmus-rt.org/tutor16/manual.html)

Some additional information is available on the LITMUS^RT wiki at:

- Some notes on the [user-space interface](https://wiki.litmus-rt.org/litmus/UserspaceTools).

- General [installation instructions](https://wiki.litmus-rt.org/litmus/InstallationInstructions).

When writing real-time applications that need to make use of
LITMUS^RT-specific interfaces and facilities, the application binary
needs to be linked against `liblitmus` (this library). Instructions for
how to do so are provided on the LITMUS^RT wiki.

- HOWTO: [linking against liblitmus](https://wiki.litmus-rt.org/litmus/LinkAgainstLiblitmusTutorial).

As of version 2016.1, LITMUS^RT supports reservation-based scheduling.
How to set up and use reservations is described in a separate document.

- HOWTO: [working with reservations](doc/howto-use-resctl.md)


Getting Help
------------

For any questions, bug reports, suggestions, etc. pertaining to
LITMUS^RT or `liblitmus`, please contact the [LITMUS^RT mailing
list](https://wiki.litmus-rt.org/litmus/Mailinglist).


Tools and Programs in `liblitmus`
---------------------------------

### setsched

Run as:

    setsched [<PLUGIN>]
    
Selects the active scheduler. Must be run as root. Run without argument
to be presented with a list of available plugins (requires 'dialog' to
be installed).

### showsched

Run as:

    showsched

Print the name of the currently active scheduler.

### rt_launch

Run as:

    rt_launch  [-w] [-p <PARTITION>] <WCET> <PERIOD> <PROGRAM> <ARGS>...

Launch the program `<PROGRAM>` as a real-time task provisioned with the
given worst-case execution time and priod. Any additional parameters are
passed on to the real-time task. The -w option makes the task wait for a
sytem release. Run `rt_launch -h` for further options.

Tip: for debugging purposes, a couple of

    rt_launch $EXEC_TIME $PERIOD find /
    
real-time processes, for reasonable values of `$EXEC_TIME` and
`$PERIOD`, generates a workload that stresses the wakeup/suspend path of
the scheduler pretty well. Similarly, running

    rt_launch $EXEC_TIME $PERIOD sha256sum `find /home | sort -R

produces a CPU-intensive workload that also includes significant IO.

### rt_spin

Run as:

    rtspin [-w] [-p <PARTITION>] WCET PERIOD DURATION

A simple spin loop for emulating purely CPU-bound workloads. Not very
realistic, but a good tool for debugging. The `-w` option makes the task
wait for a sytem release. Run `rtspin -h` for further options.

The parameters `WCET` and `PERIOD` must be given in milliseconds, the
paramter `DURATION` must be given in seconds.

### release_ts

Run as:

    release_ts [-f <NUM_TASKS>]

Release the task system. This allows for synchronous task system
releases (i.e., ensure that all tasks share a common "time zero"). The
`-f` option makes `release_ts` wait until the number of tasks waiting
for the task system release equals `<NUM_TASKS>`.

See `release_ts -h` for further options.


### Other tools

* `measure_syscall`: A simple tool that measures the cost of invoking a
  LITMUS^RT system call.

* `cycles`: Display measured cycles per time interval, as determined by
  the cycle counter. Useful for converting benchmarking results.

* `base_task`: Example real-time task. To be used as a template for the
  development of single-threaded real-time tasks.

* `base_mt_task`: Example multi-threaded real-time task. To be used as a
  template for the development of multithreaded real-time tasks.

* `uncache`: Demo application showing how to allocate and use uncached
  pages (i.e., pages that bypass the cache).

* `runtests`: The LITMUS^RT test suite. By default, it runs the tests
  for the currently active plugin. Use this frequently when hacking on the
  core plugins. Add support for new plugins as needed. The tests can be
  found in [tests/](tests/) directory.
