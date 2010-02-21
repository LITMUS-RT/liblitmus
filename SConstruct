# #####################################################################
# User configuration.
LITMUS_KERNEL = '../litmus2010'

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

KERNEL_INCLUDE = '%s/include/' % LITMUS_KERNEL
INCLUDE_DIRS = 'include/ ' + KERNEL_INCLUDE

# #####################################################################
# Build checks.

nrSrc = """#include <linux/unistd.h>
int main(int argc, char **argv)
{
  return __NR_set_rt_task_param;
}
"""

def CheckASMLink(context):
    context.Message('Checking for asm/ link in kernel include/... ')
    result = context.TryLink(nrSrc, '.c')
    context.Result(result)
    return result

# #####################################################################
# Build configuration.
from os import uname, environ

# sanity check
(ostype, _, _, _, arch) = uname()
if ostype != 'Linux':
    print 'Error: Building liblitmus is only supported on Linux.'
    Exit(1)

# override arch if ARCH is set in environment or command line
if 'ARCH' in ARGUMENTS:
    arch = ARGUMENTS['ARCH']
elif 'ARCH' in environ:
    arch = environ['ARCH']

if arch not in SUPPORTED_ARCHS:
    print 'Error: Building ft_tools is only supported for the following', \
        'architectures: %s.' % ', '.join(sorted(SUPPORTED_ARCHS))
    Exit(1)
else:
    arch_flags = Split(SUPPORTED_ARCHS[arch])

# add architecture dependent include search path
if arch in ['x86','x86_64']:
    include_arch = 'x86'
else:
    include_arch = 'sparc'

KERNEL_ARCH_INCLUDE = '%s/arch/%s/include' % (LITMUS_KERNEL, include_arch)
INCLUDE_DIRS = INCLUDE_DIRS + ' ' + KERNEL_ARCH_INCLUDE

# Set Environment
env = Environment(
    CC = 'gcc',
    CPPPATH = Split(INCLUDE_DIRS),
    CCFLAGS = Split(DEBUG_FLAGS) + Split(API_FLAGS) + arch_flags,
    LINKFLAGS = arch_flags,
)

# Check compile environment
if not env.GetOption('clean'):
    print 'Building %s binaries.' % arch
    # Check for kernel headers.
    conf = Configure(env, custom_tests = {'CheckASMLink' : CheckASMLink})
    if not conf.CheckCHeader('litmus/rt_param.h'):
	print 'Env CCFLAGS = %s' % env['CCFLAGS']
	print 'Env CPPPATH = %s' % env['CPPPATH']
        print "Error: Canot find kernel headers in '%s'." % LITMUS_KERNEL
        print "Please ensure that LITMUS_KERNEL in SConstruct", \
            "contains a valid path."
        Exit(1)
    if not conf.CheckASMLink():
	print 'Env CCFLAGS = %s' % env['CCFLAGS']
	print 'Env CPPPATH = %s' % env['CPPPATH']
        print "Error: The LITMUS^RT syscall numbers are not available."
        print "Please ensure sure that the kernel in '%s' is configured." \
            % LITMUS_KERNEL
        Exit(1)
    env = conf.Finish()

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
# Targets: liblitmus libst
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

mkc = Builder(action = 'tests/make_catalog.py $SOURCE > $TARGET')
test = mtrt.Clone()
test.Append(BUILDERS = {'TestCatalog' : mkc})
test.Append(CPPPATH = ['tests/'])

catalog = test.TestCatalog('tests/__test_catalog.inc', Glob('tests/*.c'))
test.Program('runtests', Glob('tests/*.c'))
