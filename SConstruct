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
from ConfigureJNI import ConfigureJNI

VariantDir('build/src', 'src', duplicate=0)
VariantDir('build/test', 'test', duplicate=0)

AddOption('--enable-debug',
          dest = 'DEBUG',
          action = 'store_true',
          help = 'Enable compilation with debugging symbols')

AddOption('--enable-profiling',
          dest = 'PROFILING',
          action = 'store_true',
          help = 'Enable compilation with profiling information')

AddOption('--enable-statistics',
          dest = 'STATISTICS',
          action = 'store_true',
          help = 'Enable compilation with statistics')

AddOption('--target',
          dest = 'TARGET',
          metavar = 'TARGET',
          type = 'string',
          help = 'Compile for target TARGET')

env = Environment(CPPFLAGS = '-Werror -Wextra -Wall -pedantic -std=c99 -D_POSIX_C_SOURCE=199506L -DAIO4C_P_TYPE=int',
                  ENV = {'PATH': os.environ['PATH']})

if 'TMP' in os.environ:
    env.Append(ENV, {'TMP': os.environ['TMP']})

if 'JNI_CPPPATH' not in os.environ:
    if not ConfigureJNI(env):
        print "Java Native Interface required, exiting..."
        Exit(1)
else:
    env.Append(CPPPATH = os.environ['JNI_CPPPATH'].split())

if GetOption('DEBUG'):
    env.Append(CCFLAGS = '-ggdb3')
    env.Append(LINKFLAGS = '-ggdb3')

if GetOption('PROFILING'):
    env.Append(CCFLAGS = '-pg')
    env.Append(LINKFLAGS = '-pg')

if GetOption('STATISTICS'):
    env.Append(CPPDEFINES = {"AIO4C_ENABLE_STATS" : 1})

if sys.platform == 'cygwin':
    env['CC'] = 'i686-w64-mingw32-gcc'

if GetOption('TARGET'):
    env['CC'] = GetOption('TARGET') + "-gcc"

libs = []

if sys.platform == 'win32' or sys.platform == 'cygwin' or (GetOption('TARGET') and 'mingw' in GetOption('TARGET')):
    w32env = env.Clone()
    w32env['SHOBJSUFFIX'] = '.obj'
    w32env['SHLIBSUFFIX'] = '.dll'
    w32env['PROGSUFFIX'] = '.exe'
    w32env['OBJSUFFIX'] = '.obj'
    if '-fPIC' in w32env['SHCCFLAGS']:
        w32env['SHCCFLAGS'].remove('-fPIC')
    w32env.Append(CPPDEFINES = {"AIO4C_WIN32": 1, "_WIN32_WINNT": 0x0600, "WINVER": 0x0600})
    libs.append('ws2_32')
    envlib = w32env.Clone()
    envuser = w32env.Clone()
    envlib.Append(CPPDEFINES = {"AIO4C_DLLEXPORT": "'__declspec(dllexport)'"})
    envuser.Append(CPPDEFINES = {"AIO4C_DLLIMPORT": "'__declspec(dllimport)'"})
else:
    envlib = env.Clone()
    envuser = env.Clone()
    libs.append('pthread')

env.Java('build/java', 'java')
env.JavaH(target = File('include/aio4c/jni/client.h'), source = 'build/java/com/aio4c/Client.class', JAVACLASSDIR = 'build/java')

libfiles = Glob('build/src/*.c')

if not GetOption('STATISTICS'):
    libfiles.remove(File('build/src/stats.c'))

envlib.SharedLibrary('build/aio4c', libfiles + Glob('build/src/jni/*.c'), LIBS=libs, CPPPATH=['include'])
envuser.Program('build/client', 'build/test/client.c', LIBS=['aio4c'], LIBPATH='build', CPPPATH=['include'])
envuser.Program('build/server', 'build/test/server.c', LIBS=['aio4c'], LIBPATH='build', CPPPATH=['include'])
