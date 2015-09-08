LITMUS^RT User-Space Library: liblitmus
=======================================

Copyright (c) 2007-2015 The LITMUS^RT Project & Contributors
http://www.litmus-rt.org

Released as open source under the terms of the GNU General Public License
(GPL2).

Overview
========
This library and the included tools provide the user-space interface to
LITMUS^RT. Real-time tasks should link against this library. The header
file "litmus.h" contains all necessary system calls and definitions to
interact with the kernel services provided for real-time tasks.

Tools and Programs
==================

* setsched [<PLUGIN>]
  Select the active scheduler. Must be run as root. Run without argument to be
  presented with a list of available plugins (requires 'dialog' to be
  installed).

* showsched
  Print the name of the currently active scheduler.

* rt_launch  [-w] [-p <PARTITION>] <WCET> <PERIOD> <PROGRAM> <ARGS>...
  Launch the program <PROGRAM> as a real-time task provisioned with the
  given worst-case execution time and priod. Any additional parameters
  are passed on to the real-time task. The -w option makes the task wait
  for a sytem release. Run rt_launch -h for further options.

  Tip: for debugging purposes, a couple of "rt_launch $EXE $PER find /",
  for reasonable values of $EXE and $PER, generates a workload that
  stresses the wakeup/suspend path of the scheduler pretty well.
  Similarly, running "rt_launch $EXE $PER sha256sum `find /home | sort
  -R`" produces a CPU-intensive workload that also includes significant
  IO.

* rtspin [-w] [-p <PARTITION>] [-c CLASS] WCET PERIOD DURATION
  A simple spin loop for emulating purely CPU-bound workloads. Not very
  realistic, but a good tool for debugging. The -w option makes the task
  wait for a sytem release. Run rtspin -h for further options.

* release_ts
  Release the task system. This allows for synchronous task system
  releases (i.e., ensure that all tasks share a common "time zero").

* measure_syscall
  A simple tool that measures the cost of a system call.

* cycles
  Display measured cycles per time interval, as determined by the cycle
  counter. Useful for converting benchmarking results.

* base_task
  Example real-time task. Can be used as a basis for the development
  of single-threaded real-time tasks.

* base_mt_task
  Example multi-threaded real-time task. Use as a basis for the
  development of multithreaded real-time tasks.

* uncache
  Demo application showing how to allocate and use uncached pages (i.e.,
  pages that bypass the cache).

* runtests
  The LITMUS^RT test suite. By default, it runs the tests for the
  currently active plugin. Use this frequently when hacking on the core
  plugins. Add support for new plugins as needed. The tests can be found
  in tests/ directory.

Further Reading
===============

Some additional information is available on the LITMUS^RT Wiki at:

- https://wiki.litmus-rt.org/litmus/UserspaceTools

- https://wiki.litmus-rt.org/litmus/InstallationInstructions

- https://wiki.litmus-rt.org/litmus/LinkAgainstLiblitmusTutorial
