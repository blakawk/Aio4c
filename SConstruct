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

env = Environment(CPPFLAGS = '-Werror -Wextra -Wall -pedantic -std=c99 -D_POSIX_C_SOURCE=199506L -DAIO4C_P_TYPE=int',
                  ENV = {'PATH': os.environ['PATH'], 'TMP': os.environ['TMP']})

if not ConfigureJNI(env):
    print "Java Native Interface required, exiting..."
    Exit(1)

for path in env['JNI_CPPPATH']:
    env.Append(CPPFLAGS = ' -I%s' % path)

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

libs = ['pthread']

if sys.platform == 'win32' or sys.platform == 'cygwin':
    env.Append(CPPDEFINES = {"AIO4C_WIN32": 1, "_WIN32_WINNT": 0x0600, "WINVER": 0x0600})
#    env.Append(LINKFLAGS = '-Wl,--enable-runtime-pseudo-reloc -Wl,--no-undefined')
    libs.append('ws2_32')

env.Java('build/java', 'java')
env.JavaH(target = File('include/aio4c/jni/client.h'), source = 'build/java/com/aio4c/Client.class', JAVACLASSDIR = 'build/java')


envaio4c = env.Clone()
envaio4c.Append(CPPDEFINES = {"AIO4C_BUILD": 1})

libaio4c = envaio4c.SharedLibrary('build/aio4c', Glob('build/src/*.c') + Glob('build/src/jni/*.c'), LIBS=libs, CPPPATH=['include'])
client = env.Program('build/client', 'build/test/client.c', LIBS=['aio4c'], LIBPATH='build', CPPPATH=['include'])
server = env.Program('build/server', 'build/test/server.c', LIBS=['aio4c'], LIBPATH='build', CPPPATH=['include'])
