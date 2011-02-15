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

VariantDir('build/src', 'src', duplicate=0)
VariantDir('build/test', 'test', duplicate=0)

env = Environment(CCFLAGS = '-g',
                  LDFLAGS = '-i',
                  CPPFLAGS = '-Werror -Wall -pedantic -std=c99 -D_POSIX_C_SOURCE=199506L')

env.SharedLibrary('build/aio4c', Glob('build/src/*.c'), LIBS=['pthread'], CPPPATH=['include'])
env.Program('build/client', 'build/test/client.c', LIBS=['aio4c'], LIBPATH='build', CPPPATH=['include'])

