# #####################################################################
# User configuration.
LITMUS_KERNEL = '../litmus2008'

# #####################################################################
# Internal configuration.
DEBUG_FLAGS  = '-Wall -g -Wdeclaration-after-statement'
API_FLAGS    = '-D_XOPEN_SOURCE=600 -D_GNU_SOURCE'

INCLUDE_DIRS = 'include/  %s/include/' % LITMUS_KERNEL

# #####################################################################
# Build configuration.
from os import uname

# sanity check
(os, _, _, _, arch) = uname()
if os != 'Linux':
    print 'Error: Building liblitmus is only supported on Linux.'
    Exit(1)

if arch not in ('sparc64', 'i686'):
    print 'Error: Building liblitmus is only supported on i686 and sparc64.'
    Exit(1)

env = Environment(
    CC = 'gcc',
    CPPPATH = Split(INCLUDE_DIRS),
    CCFLAGS = Split(DEBUG_FLAGS) + Split(API_FLAGS)
)

if arch == 'sparc64':
    # build 64 bit sparc v9 binaries
    v9 = Split('-mcpu=v9 -m64')
    env.Append(CCFLAGS = v9, LINKFLAGS = v9)

# link with lib libtmus
rt = env.Clone(
    LIBS     = 'litmus',
    LIBPATH  = '.'
)
rt.Append(LINKFLAGS = '-static')

# multithreaded real-time tasks
mtrt = rt.Clone()
mtrt.Append(LINKFLAGS = '-pthread')


# #####################################################################
# Targets: liblitmus
# All the files in src/ are part of the library.
env.Library('litmus',
            ['src/kernel_iface.c', 'src/litmus.c', 'src/sched_trace.c',
             'src/syscalls.c',   'src/task.c'])

# #####################################################################
# Targets: simple tools that do not depend on liblitmus
env.Program('cycles', 'bin/cycles.c')

# #####################################################################
# Targets: tools that depend on liblitmus
rt.Program('base_task', 'bin/base_task.c')
mtrt.Program('base_mt_task', 'bin/base_mt_task.c')
rt.Program('wait_test', 'bin/wait_test.c')
rt.Program('mode_test', 'bin/mode_test.c')
rt.Program('np_test', 'bin/np_test.c')
rt.Program('rt_launch', ['bin/rt_launch.c', 'bin/common.c'])
rt.Program('rtspin', ['bin/rtspin.c', 'bin/common.c'])
rt.Program('release_ts', 'bin/release_ts.c')
rt.Program('showst', 'bin/showst.c')
