Help("""
=============================================
liblitmus --- The LITMUS^RT Userspace Library

There are a number of user-configurable build
variables. These can either be set on the
command line (e.g., scons ARCH=x86) or read
from a local configuration file (.config).

Run 'scons --dump-config' to see the final
build configuration.

""")

import os
(ostype, _, _, _, arch) = os.uname()

# sanity check
if ostype != 'Linux':
    print 'Error: Building liblitmus is only supported on Linux.'
    Exit(1)


# #####################################################################
# Internal configuration.
DEBUG_FLAGS  = '-Wall -g -Wdeclaration-after-statement'
API_FLAGS    = '-D_XOPEN_SOURCE=600 -D_GNU_SOURCE'
X86_32_FLAGS = '-m32'
X86_64_FLAGS = '-m64'
V9_FLAGS     = '-mcpu=v9 -m64'

SUPPORTED_ARCHS = {
    'sparc64'	: V9_FLAGS,
    'x86'	: X86_32_FLAGS,
    'x86_64'	: X86_64_FLAGS,
}

ARCH_ALIAS = {
    'i686'      : 'x86'
}

# name of the directory that has the arch headers in the Linux source
INCLUDE_ARCH = {
    'sparc64'   : 'sparc',
    'x86'       : 'x86',
    'x86_64'    : 'x86',
}

INCLUDE_DIRS = [
    # library headers
    'include/',
    # Linux kernel headers
    '${LITMUS_KERNEL}/include/',
    # Linux architecture-specific kernel headers
    '$LITMUS_KERNEL/arch/${INCLUDE_ARCH}/include'
    ]

# #####################################################################
# User configuration.

vars = Variables('.config', ARGUMENTS)

vars.AddVariables(
    PathVariable('LITMUS_KERNEL',
                 'Where to find the LITMUS^RT kernel.',
                 '../litmus2010'),

    EnumVariable('ARCH',
                 'Target architecture.',
                 arch,
                 SUPPORTED_ARCHS.keys() + ARCH_ALIAS.keys()),
)

AddOption('--dump-config',
          dest='dump',
          action='store_true',
          default=False,
          help="dump the build configuration and exit")

# #####################################################################
# Build configuration.

env  = Environment(variables = vars)

# Check what we are building for.
arch = env['ARCH']

# replace if the arch has an alternative name
if arch in ARCH_ALIAS:
    arch = ARCH_ALIAS[arch]
    env['ARCH'] = arch

# Get include directory for arch.
env['INCLUDE_ARCH'] = INCLUDE_ARCH[arch]

arch_flags = Split(SUPPORTED_ARCHS[arch])
dbg_flags  = Split(DEBUG_FLAGS)
api_flags  = Split(API_FLAGS)

# Set up environment
env.Replace(
    CC = 'gcc',
    CPPPATH = INCLUDE_DIRS,
    CCFLAGS = dbg_flags + api_flags + arch_flags,
    LINKFLAGS = arch_flags,
)

def dump_config(env):
    def dump(key):
        print "%15s = %s" % (key, env.subst("${%s}" % key))

    dump('ARCH')
    dump('LITMUS_KERNEL')
    dump('CPPPATH')
    dump('CCFLAGS')
    dump('LINKFLAGS')

if GetOption('dump'):
    print "\n"
    print "Build Configuration:"
    dump_config(env)
    print "\n"
    Exit(0)

# #####################################################################
# Build checks.

def CheckSyscallNr(context):
    context.Message('Checking for LITMUS^RT syscall numbers... ')
    nrSrc = """
#include <linux/unistd.h>
int main(int argc, char **argv)
{
  return __NR_set_rt_task_param;
}
"""
    result = context.TryLink(nrSrc, '.c')
    context.Result(result)
    return result


def abort(msg, help=None):
    print "Error: %s" % env.subst(msg)
    print "-" * 80
    print "This is the build configuration in use:"
    dump_config(env)
    if help:
        print "-" * 80
        print env.subst(help)
    print "\n"
    Exit(1)

# Check compile environment
if not (env.GetOption('clean') or env.GetOption('help')):
    print env.subst('Building ${ARCH} binaries.')
    # Check for kernel headers.
    conf = Configure(env, custom_tests = {'CheckSyscallNr' : CheckSyscallNr})

    conf.CheckCHeader('linux/unistd.h') or \
        abort("Cannot find kernel headers in '$LITMUS_KERNEL'",
              "Please ensure that LITMUS_KERNEL in .config is set to a valid path.")

    conf.CheckCHeader('litmus/rt_param.h') or \
        abort("Cannot find LITMUS^RT headers in '$LITMUS_KERNEL'",
              "Please ensure sure that the kernel in '$LITMUS_KERNEL'"
              " is a LITMUS^RT kernel.")

    conf.CheckSyscallNr() or \
        abort("The LITMUS^RT syscall numbers are not available.",
              "Please ensure sure that the kernel in '$LITMUS_KERNEL'"
              " is a LITMUS^RT kernel.")

    env = conf.Finish()

# #####################################################################
# Derived environments

# link with liblitmus
rt = env.Clone(
    LIBS     = Split('litmus rt'),
    LIBPATH  = '.'
)
rt.Append(LINKFLAGS = '-static')


# link with math lib
rtm = rt.Clone()
rtm.Append(LIBS = ['m'])

# multithreaded real-time tasks
mtrt = rt.Clone()
mtrt.Append(LINKFLAGS = '-pthread')

# #####################################################################
# Targets: liblitmus
# All the files in src/ are part of the library.
env.Library('litmus',
            ['src/kernel_iface.c', 'src/litmus.c',
             'src/syscalls.c', 'src/task.c'])

# #####################################################################
# Targets: simple tools that do not depend on liblitmus
env.Program('cycles', 'bin/cycles.c')

# #####################################################################
# Targets: tools that depend on liblitmus
rt.Program('base_task', 'bin/base_task.c')
mtrt.Program('base_mt_task', 'bin/base_mt_task.c')
rt.Program('rt_launch', ['bin/rt_launch.c', 'bin/common.c'])
rt.Program('rtspin', ['bin/rtspin.c', 'bin/common.c'])
rt.Program('release_ts', 'bin/release_ts.c')
rtm.Program('measure_syscall', 'bin/null_call.c')


# #####################################################################
# Test suite.

mkc = Builder(action = 'tests/make_catalog.py $SOURCES > $TARGET')
test = mtrt.Clone()
test.Append(BUILDERS = {'TestCatalog' : mkc})
test.Append(CPPPATH = ['tests/'])

catalog = test.TestCatalog('tests/__test_catalog.inc', Glob('tests/*.c'))
test.Program('runtests', Glob('tests/*.c'))

# #####################################################################
# Additional Help

Help("Build Variables\n")
Help("---------------\n")
Help(vars.GenerateHelpText(env))

