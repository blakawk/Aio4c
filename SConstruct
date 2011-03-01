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

def JavaHGenerator(source, target, env, for_signature):
    return 'javah -classpath build/java -o %s %s' % (str(target[0]), string.replace(string.replace(string.replace(str(source[0]), "java/", ""), "/", "."), ".java", ""))

javah = Builder(generator = JavaHGenerator)

env = Environment(CPPFLAGS = '-Werror -Wextra -Wall -pedantic -std=c99 -D_POSIX_C_SOURCE=199506L -DAIO4C_P_TYPE=int',
                  BUILDERS = {'JavaH': javah})

if GetOption('DEBUG'):
    env.Append(CCFLAGS = '-ggdb3')
    env.Append(LINKFLAGS = '-ggdb3')

if GetOption('PROFILING'):
    env.Append(CCFLAGS = '-pg')
    env.Append(LINKFLAGS = '-pg')

if GetOption('STATISTICS'):
    env.Append(CPPDEFINES = {"AIO4C_ENABLE_STATS" : 1})

env.Java('build/java', 'java')
env.JavaH('include/aio4c/jni/client.h', 'java/com/aio4c/Client.java')

libaio4c = env.SharedLibrary('build/aio4c', Glob('build/src/*.c') + Glob('build/src/jni/*.c'), LIBS=['pthread','rt'], CPPPATH=['include'])
client = env.Program('build/client', 'build/test/client.c', LIBS=['aio4c'], LIBPATH='build', CPPPATH=['include'])
server = env.Program('build/server', 'build/test/server.c', LIBS=['aio4c'], LIBPATH='build', CPPPATH=['include'])
