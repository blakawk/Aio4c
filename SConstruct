# Copyright © 2011 blakawk <blakawk@gentooist.com>
# All rights reserved.  Released under GPLv3 License.
#
# This program is free software: you can redistribute
# it  and/or  modify  it under  the  terms of the GNU.
# General  Public  License  as  published by the Free
# Software Foundation, version 3 of the License.
#
# This  program  is  distributed  in the hope that it
# will be useful, but  WITHOUT  ANY WARRANTY; without
# even  the  implied  warranty  of MERCHANTABILITY or
# FITNESS FOR A PARTICULAR PURPOSE.
#.
# See   the  GNU  General  Public  License  for  more
# details. You should have received a copy of the GNU
# General Public License along with this program.  If
# not, see <http://www.gnu.org/licenses/>.
import os
import string
import sys

VariantDir('build/src', 'src', duplicate=0)
VariantDir('build/test', 'test', duplicate=0)

AddOption('--enable-debug',
          dest = 'DEBUG',
          action = 'store_true',
          help = 'Enable compilation with debugging symbols')

AddOption('--enable-debug-threads',
          dest = 'DEBUG_THREADS',
          action = 'store_true',
          help = 'Enable threads debugging')

AddOption('--enable-statistics',
          dest = 'STATISTICS',
          action = 'store_true',
          help = 'Enable compilation with statistics')

AddOption('--use-select',
          dest = 'USE_POLL',
          action = 'store_false',
          default = True,
          help = 'Force use of select instead of poll')

AddOption('--use-selector-udp',
          dest = 'USE_PIPE',
          action = 'store_false',
          default = True,
          help = 'Use UDP sockets for selector instead of anonymous pipe')

AddOption('--target',
          dest = 'TARGET',
          metavar = 'TARGET',
          help = 'Compile for target TARGET')

env = Environment(CPPFLAGS = '-Werror -Wextra -Wall -pedantic -std=c99 -D_POSIX_C_SOURCE=199506L',
                  ENV = {'PATH': os.environ['PATH']})

ptr_size_src = """
#include <stdio.h>

int main(void) {
    if (sizeof(void*) == sizeof(int)) {
        printf("int");
    } else if (sizeof(void*) == sizeof(long)) {
        printf("long");
    } else {
        return 1;
    }
    return 0;
}
"""

def CheckPointerSize(context):
    context.Message('Checking size of void*... ')
    result = context.TryRun(ptr_size_src, '.c')
    if result[0]:
        context.Result(result[1])
    else:
        context.Result(result[0])
    return result

have_poll_src = """
#ifdef AIO4C_WIN32
#include <winsock2.h>
#else
#include <poll.h>
#endif

int main(void) {
    struct pollfd polls[1] = { { .fd = 0, .events = POLLIN, .revents = 0 } };
#ifndef AIO4C_WIN32
    poll(polls, 1, -1);
#else
    WSAPoll(polls, 1, -1);
#endif
    return 0;
}
"""

def HavePoll(context):
    context.Message('Checking if poll is useable... ')
    result = context.TryLink(have_poll_src, '.c', )
    context.Result(result)
    return result

have_pipe_src = """
#include <unistd.h>

int main(void) {
    int fds[2];
    pipe(fds);
    return 0;
}
"""

def HavePipe(context):
    context.Message('Checking if pipe is useable... ')
    result = context.TryLink(have_pipe_src, '.c')
    context.Result(result)
    return result

def doConfigure(env):
    if not env.GetOption('clean'):
        conf = Configure(env, custom_tests = {'CheckPointerSize': CheckPointerSize, 'HavePoll': HavePoll, 'HavePipe': HavePipe})
        result = conf.CheckPointerSize()
        if not result[0]:
            if env.GetOption('TARGET'):
                target = env.GetOption('TARGET')
                if 'x86_64' in target:
                    env.Append(CPPDEFINES = {'AIO4C_P_TYPE': 'long'})
                else:
                    env.Append(CPPDEFINES = {'AIO4C_P_TYPE': 'int'})
        else:
            env.Append(CPPDEFINES = {'AIO4C_P_TYPE': result[1]})
        if env.GetOption('USE_POLL') and conf.HavePoll():
            env.Append(CPPDEFINES = {'AIO4C_HAVE_POLL': 1})
        if env.GetOption('USE_PIPE') and conf.HavePipe():
            env.Append(CPPDEFINES = {'AIO4C_HAVE_PIPE': 1})
        conf.Finish()
    return env

if 'TMP' in os.environ:
    env.Append(ENV = {'TMP': os.environ['TMP']})

if not GetOption('clean'):
    if 'JAVA_HOME' not in os.environ:
        print 'Please set JAVA_HOME to the root of your Java SDK'
        Exit(1)
    else:
        jni_path = "%s/include" % os.path.normpath(os.environ['JAVA_HOME'])
        jni_lib_path = "%s/lib" % os.path.normpath(os.environ['JAVA_HOME'])
        env.Append(LIBPATH = [jni_lib_path])
        env.Append(CPPPATH = [jni_path])

if GetOption('DEBUG'):
    env.Append(CCFLAGS = '-g')
    env.Append(LINKFLAGS = '-g')

if GetOption('DEBUG_THREADS'):
    env.Append(CPPDEFINES = {"AIO4C_DEBUG_THREADS": 1})

if GetOption('STATISTICS'):
    env.Append(CPPDEFINES = {"AIO4C_ENABLE_STATS" : 1})

if GetOption('TARGET'):
    env['CC'] = GetOption('TARGET') + "-gcc"

env.Append(CPPPATH = ['include'])

if sys.platform == 'win32' or (GetOption('TARGET') and 'mingw' in GetOption('TARGET')):
    AddOption('--windows-version',
            dest = 'WINVER',
            metavar = 'WINVER',
            choices = ['XP', 'VISTA'],
            default = 'VISTA',
            help = 'Compile for windows version XP or VISTA (default: %default)')

    if GetOption('WINVER') == 'XP':
        winver = '0x0501'
    elif GetOption('WINVER') == 'VISTA':
        winver = '0x0600'

    w32env = env.Clone()

    if not GetOption('clean') and not os.path.exists("%s/win32" % jni_path):
        print "JAVA_HOME = %s does not seems to be a win32 SDK" % os.environ['JAVA_HOME']
        Exit(1)

    if not GetOption('clean'):
        w32env.Append(CPPPATH = ["%s/win32" % jni_path])

    if sys.platform != 'win32':
        w32env['LIBPREFIX'] = '';
        w32env['SHOBJSUFFIX'] = '.obj'
        w32env['SHLIBSUFFIX'] = '.dll'
        w32env['PROGSUFFIX'] = '.exe'
        w32env['OBJSUFFIX'] = '.obj'
        if '-fPIC' in w32env['SHCCFLAGS']:
            w32env['SHCCFLAGS'].remove('-fPIC')
    elif not GetOption('clean'):
        env.Append(ENV = {'PATH': "%s/bin:%s" % (os.path.normpath(os.environ['JAVA_HOME']), os.environ['PATH'])})

    if int(winver, 16) < 0x0600:
        w32env.Append(CPPDEFINES = {"FD_SETSIZE": 4096})

    w32env.Append(CPPDEFINES = {"AIO4C_WIN32": 1, "WINVER": winver, "_WIN32_WINNT": winver})
    w32env.Append(LIBS = ['ws2_32'])

    w32env = doConfigure(w32env)

    envj = w32env.Clone()
    envlib = w32env.Clone()
    envuser = w32env.Clone()

    envlib.Append(CPPDEFINES = {"AIO4C_DLLEXPORT": "'__declspec(dllexport)'"})
    envuser.Append(CPPDEFINES = {"AIO4C_DLLIMPORT": "'__declspec(dllimport)'"})
else:
    if not GetOption('clean') and not os.path.exists("%s/linux" % jni_path):
        print "JAVA_HOME = %s does not seems to be a linux SDK" % os.environ['JAVA_HOME']
        Exit(1)

    if not GetOption('clean'):
        env.Append(CPPPATH = ["%s/linux" % jni_path])
        env.Append(ENV = {'PATH': "%s/bin:%s" % (os.path.normpath(os.environ['JAVA_HOME']), os.environ['PATH'])})

    env.Append(LIBS = ['pthread'])

    env = doConfigure(env)

    envj = env.Clone()
    envlib = env.Clone()
    envuser = env.Clone()

envj.Java('build/java', 'java')
envj.JavaH(target = File('include/aio4c/jni/client.h'), source = 'build/java/com/aio4c/Client.class', JAVACLASSDIR = 'build/java')

libfiles = Glob('build/src/*.c')

if not GetOption('STATISTICS'):
    libfiles.remove(File('build/src/stats.c'))

envlib.SharedLibrary('build/aio4c', libfiles + Glob('build/src/jni/*.c'), CPPPATH=env['CPPPATH'])
envuser.Program('build/client', 'build/test/client.c', LIBS=['aio4c'], LIBPATH='build', CPPPATH=env['CPPPATH'])
envuser.Program('build/server', 'build/test/server.c', LIBS=['aio4c'], LIBPATH='build', CPPPATH=env['CPPPATH'])
