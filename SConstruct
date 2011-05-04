# -*- coding: utf-8 -*-
#
# Copyright (c) 2011 blakawk
#
# This file is part of Aio4c <http://aio4c.so>.
#
# Aio4c <http://aio4c.so> is free software: you
# can  redistribute  it  and/or modify it under
# the  terms  of the GNU General Public License
# as published by the Free Software Foundation,
# version 3 of the License.
#
# Aio4c <http://aio4c.so> is distributed in the
# hope  that it will be useful, but WITHOUT ANY
# WARRANTY;  without  even the implied warranty
# of   MERCHANTABILITY   or   FITNESS   FOR   A
# PARTICULAR PURPOSE.
#
# See  the  GNU General Public License for more
# details.  You  should have received a copy of
# the  GNU  General  Public  License along with
# Aio4c    <http://aio4c.so>.   If   not,   see
# <http://www.gnu.org/licenses/>.

import os
import string
import sys

VariantDir('build/src', 'src', duplicate=0)
VariantDir('build/test', 'test', duplicate=0)

AddOption('--disable-debug',
          dest = 'DEBUG',
          action = 'store_false',
          default = True,
          help = 'Disable compilation with debugging symbols')

AddOption('--enable-debug-threads',
          dest = 'DEBUG_THREADS',
          action = 'store_true',
          help = 'Enable threads debugging')

AddOption('--disable-statistics',
          dest = 'STATISTICS',
          action = 'store_false',
          default = True,
          help = 'Disable compilation with statistics')

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

AddOption('--disable-condition',
          dest = 'USE_CONDITION',
          action = 'store_false',
          default = True,
          help = 'Disable use of condition variables')

AddOption('--target',
          dest = 'TARGET',
          metavar = 'TARGET',
          help = 'Compile for target TARGET')

AddOption('--enable-profiling',
          dest = 'PROFILING',
          action = 'store_true',
          help = 'Compile with profiling information')

AddOption('--use-profiling-information',
          dest = 'USE_PROFILING_INFO',
          action = 'store_true',
          help = 'Optimizes using profiling information')

AddOption('--max-threads',
          dest = 'NUM_THREADS',
          metavar = 'NUM_THREADS',
          type = int,
          default = 4096,
          help = 'Allow NUM_THREADS creation (default: %default)')

AddOption('--use-java-home',
          dest = 'IGNORE_JAVA_HOME',
          action = 'store_false',
          default = True,
          help = 'Try to build JNI using JDK indicated in JAVA_HOME environment variable')

AddOption('--disable-documentation-generation',
          dest = 'GENERATE_DOC',
          action = 'store_false',
          default = True,
          help = 'Disable documentation generation')

env = Environment(CPPFLAGS = '-Werror -Wextra -Wall -pedantic -std=c99 -D_POSIX_C_SOURCE=199506L',
                  ENV = {'PATH': os.environ['PATH']})

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
    result = context.TryLink(have_poll_src, '.c')
    context.Result(result)
    return result

have_pipe_src = """
#include <unistd.h>

int main(void) {
    int fds[2];
    if (pipe(fds) < 0) {
        return 0;
    }
    return 0;
}
"""

def HavePipe(context):
    context.Message('Checking if pipe is useable... ')
    result = context.TryLink(have_pipe_src, '.c')
    context.Result(result)
    return result

have_condition_src = """
#ifndef AIO4C_WIN32
#include <pthread.h>
#else
#include <windows.h>
#include <winbase.h>
#endif

int main(void) {
#ifndef AIO4C_WIN32
    pthread_cond_t cond = PTHREAD_COND_INITIALIZER;
    pthread_mutex_t mutex = PTHREAD_MUTEX_INITIALIZER;
    pthread_mutex_lock(&mutex);
    pthread_cond_wait(&cond, &mutex);
    pthread_mutex_unlock(&mutex);
#else
    CONDITION_VARIABLE cond;
    CRITICAL_SECTION mutex;
    InitializeConditionVariable(&cond);
    InitializeCriticalSection(&mutex);
    EnterCriticalSection(&mutex);
    SleepConditionVariableCS(&cond, &mutex, INFINITE);
    LeaveCriticalSection(&mutex);
#endif
    return 0;
}
"""

def HaveCondition(context):
    context.Message('Checking if condition variables are useable... ')
    result = context.TryLink(have_condition_src, '.c')
    context.Result(result)
    return result

pointer_size_src = """
#include <stdio.h>

int main(void) {
    printf("%d", (int)sizeof(void*));
    return 0;
}
"""
def PointerSize(context):
    context.Message('Checking the size of void*... ')
    result, output = context.TryRun(pointer_size_src, '.c')
    context.Result(output)
    return (result, output)

def doConfigure(env):
    if not env.GetOption('clean') and not env.GetOption('help'):
        conf = Configure(env, custom_tests = {'HavePoll'        : HavePoll,
                                              'HavePipe'        : HavePipe,
                                              'HaveCondition'   : HaveCondition,
                                              'CheckPointerSize': PointerSize})
        result, output = conf.CheckPointerSize()
        if not result:
            if env.GetOption('TARGET'):
                target = env.GetOption('TARGET')
                if 'x86_64' in target:
                    env.Append(CPPDEFINES = {'AIO4C_P_SIZE': '8'})
                else:
                    env.Append(CPPDEFINES = {'AIO4C_P_SIZE': '4'})
        else:
            env.Append(CPPDEFINES = {'AIO4C_P_SIZE': output})
        if env.GetOption('USE_POLL') and conf.HavePoll():
            env.Append(CPPDEFINES = {'AIO4C_HAVE_POLL': 1})
        if env.GetOption('USE_PIPE') and conf.HavePipe():
            env.Append(CPPDEFINES = {'AIO4C_HAVE_PIPE': 1})
        if env.GetOption('USE_CONDITION') and conf.HaveCondition():
            env.Append(CPPDEFINES = {'AIO4C_HAVE_CONDITION': 1})
        conf.Finish()
    return env

if 'LD_LIBRARY_PATH' in os.environ:
    env.Append(ENV = {'LD_LIBRARY_PATH': os.environ['LD_LIBRARY_PATH']})

if 'TMP' in os.environ:
    env.Append(ENV = {'TMP': os.environ['TMP']})

if not GetOption('clean') and not GetOption('help'):
    if not GetOption('IGNORE_JAVA_HOME'):
        if 'JAVA_HOME' not in os.environ:
            print 'Please set JAVA_HOME to the root of your Java SDK'
            Exit(1)
        else:
            jni_path = "%s/include" % os.path.normpath(os.environ['JAVA_HOME'])
            jni_lib_path = "%s/lib" % os.path.normpath(os.environ['JAVA_HOME'])
            env.Append(LIBPATH = [jni_lib_path])
            env.Append(CPPPATH = [jni_path])

env.Append(CCFLAGS = '-g')
env.Append(LINKFLAGS = '-g')
env.Append(LINKFLAGS = '-Wl,-no-undefined')

if not GetOption('DEBUG'):
    env.Append(CCFLAGS = '-march=native -mtune=native -O2')
    env.Append(LINKFLAGS = '-march=native -mtune=native -O2')
else:
    env.Append(CPPDEFINES = {'AIO4C_DEBUG': 1})

if GetOption('PROFILING') and GetOption('USE_PROFILING_INFO'):
    print "Options --use-profiling-information and --enable-profiling are mutually exclusives."
    Exit(1)

if GetOption('PROFILING'):
    env.Append(CCFLAGS = '-fprofile-generate')
    env.Append(LINKFLAGS = '-fprofile-generate')

if GetOption('USE_PROFILING_INFO'):
    env.Append(CCFLAGS = '-fprofile-use -fprofile-correction')
    env.Append(LINKFLAGS = '-fprofile-use -fprofile-correction')

if GetOption('DEBUG_THREADS'):
    env.Append(CPPDEFINES = {"AIO4C_DEBUG_THREADS": 1})

if GetOption('STATISTICS'):
    env.Append(CPPDEFINES = {"AIO4C_ENABLE_STATS" : 1})

if GetOption('TARGET'):
    env['CC'] = GetOption('TARGET') + "-gcc"

env.Append(CPPPATH = ['include'])
env.Append(CPPDEFINES = {"AIO4C_MAX_THREADS": GetOption('NUM_THREADS')})

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

    if not GetOption('IGNORE_JAVA_HOME'):
    	if not GetOption('clean') and not GetOption('help') and not os.path.exists("%s/win32" % jni_path):
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
    elif not GetOption('clean') and not GetOption('help') and not GetOption('IGNORE_JAVA_HOME'):
        env.Append(ENV = {'PATH': "%s/bin:%s" % (os.path.normpath(os.environ['JAVA_HOME']), os.environ['PATH'])})

    if int(winver, 16) < 0x0600:
        w32env.Append(CPPDEFINES = {"FD_SETSIZE": 4096})

    w32env.Append(CPPDEFINES = {"AIO4C_WIN32": 1, "WINVER": winver, "_WIN32_WINNT": winver})
    w32env.Append(LIBS = ['ws2_32'])

    w32env = doConfigure(w32env)

    envj = w32env.Clone()
    envlib = w32env.Clone()
    envuser = w32env.Clone()

    envlib.Append(CPPDEFINES = {"AIO4C_API": "'__declspec(dllexport)'"})
    envuser.Append(CPPDEFINES = {"AIO4C_API": "'__declspec(dllimport)'"})
else:
    if not GetOption('clean') and not GetOption('help') and not GetOption('IGNORE_JAVA_HOME') and not os.path.exists("%s/linux" % jni_path):
        print "JAVA_HOME = %s does not seems to be a linux SDK" % os.environ['JAVA_HOME']
        Exit(1)

    if not GetOption('clean') and not GetOption('help') and not GetOption('IGNORE_JAVA_HOME'):
        env.Append(CPPPATH = ["%s/linux" % jni_path])
        env.Append(ENV = {'PATH': "%s/bin:%s" % (os.path.normpath(os.environ['JAVA_HOME']), os.environ['PATH'])})

    env.Append(LIBS = ['pthread'])

    env = doConfigure(env)

    envj = env.Clone()
    envlib = env.Clone()
    envuser = env.Clone()

def JniInterfaceBuilder(target, source, env):
    cls = os.path.splitext(str(source[0]))[0].replace('build/java/','').replace('/','.')
    javah = "javah -classpath build/java -o " + str(target[0]) + " " + cls
    result = env.Execute(javah)
    if result == 0:
        file = open(target[0].get_abspath())
        found = True
        for line in file:
            if "JNICALL" in line:
                funname = line.split(' ')[3].strip()
                implname = os.path.splitext(str(target[0]).replace('include/aio4c','src'))[0]+'.c'
                impl = open(implname)
                funfound = False
                for line2 in impl:
                    if funname in line2:
                        funfound = True
                        break
                if not funfound:
                    print "error: " + funname + " defined in JNI header " + str(target[0]) + " but not implemented in " + implname
                found = found and funfound
        if not found:
            env.Exit(1)
    return result

envuser.Append(LIBS = ['aio4c'])
envj.Append(JAVACFLAGS = '-encoding UTF-8 -target 1.6 -source 1.6')
envj.Append(JAVACLASSDIR = 'build/java')
envj.Append(BUILDERS = {"JniInterface": Builder(action = JniInterfaceBuilder)})
envj.Append(JAVAVERSION = '1.6')

javafiles = []
eclipseclassfiles = []
for dir, _, file in os.walk('java'):
    for f in file:
        name = os.path.join(dir, f)
        if 'package-info.java' not in name and '.java' in name:
            javafiles.append(name)
            eclipseclassfiles.append(name.replace('.java','.class'))

javafiles.sort()
classfiles = envj.Java('build/java', javafiles, JAVASOURCEPATH = 'java')
jar = envj.Jar('build/aio4c.jar', classfiles, JARCHDIR = 'build/java')

envj.Clean(jar, eclipseclassfiles)

envj.JniInterface(target = File('include/aio4c/jni/aio4c.h'), source = 'build/java/com/aio4c/Aio4c.class')
envj.JniInterface(target = File('include/aio4c/jni/buffer.h'), source = 'build/java/com/aio4c/buffer/Buffer.class')
envj.JniInterface(target = File('include/aio4c/jni/connection.h'), source = 'build/java/com/aio4c/Connection.class')
envj.JniInterface(target = File('include/aio4c/jni/client.h'), source = 'build/java/com/aio4c/Client.class')
envj.JniInterface(target = File('include/aio4c/jni/log.h'), source = 'build/java/com/aio4c/log/Log.class')
envj.JniInterface(target = File('include/aio4c/jni/server.h'), source = 'build/java/com/aio4c/Server.class')

libfiles = Glob('build/src/*.c')

if not GetOption('STATISTICS'):
    libfiles.remove(File('build/src/stats.c'))

shlib = envlib.SharedLibrary('build/aio4c', libfiles + Glob('build/src/jni/*.c'))
envlib.Clean(shlib, 'build')

envuser.Program('build/client', 'build/test/client.c', LIBPATH='build')
envuser.Program('build/server', 'build/test/server.c', LIBPATH='build')
envuser.Program('build/queue', 'build/test/queue.c', LIBPATH='build')
envuser.Program('build/selector', 'build/test/selector.c', LIBPATH='build')
envuser.Program('build/benchmark', 'build/test/benchmark.c', LIBPATH='build')

envj.Java('build/test', 'test', JAVACLASSPATH = 'build/aio4c.jar')

def DoxygenBuilder(target, source, env):
    return env.Execute('doxygen ' + str(source[0]))

def JavadocBuilder(target, source, env):
    command = 'javadoc -private -author -classpath java -d build/doc/java -doctitle "Aio4c Java Interface" -charset "UTF-8" -keywords -sourcepath java -use -version -docencoding "UTF-8" -source 1.6'
    for s in source:
        command = command + " " + str(s)
    return env.Execute(command)

if env.GetOption('GENERATE_DOC'):
    envuser.Append(BUILDERS = {'Doxygen': Builder(action = DoxygenBuilder)})
    incfiles = Glob('include/*.h') + Glob('include/aio4c/*.h')
    doxygen = envuser.Doxygen('build/doc/core/html/index.html', ['doc/core/Doxyfile'] + incfiles)
    envuser.Clean(doxygen, 'build/doc/core/html')

    envj.Append(BUILDERS = {'Javadoc': Builder(action = JavadocBuilder)})
    javadoc = envj.Javadoc('build/doc/java/index.html', javafiles)
    envj.Clean(javadoc, 'build/doc/java')
