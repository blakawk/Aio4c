/**
 * Copyright © 2011 blakawk <blakawk@gentooist.com>
 * All rights reserved.  Released under GPLv3 License.
 *
 * This program is free software: you can redistribute
 * it  and/or  modify  it under  the  terms of the GNU.
 * General  Public  License  as  published by the Free
 * Software Foundation, version 3 of the License.
 *
 * This  program  is  distributed  in the hope that it
 * will be useful, but  WITHOUT  ANY WARRANTY; without
 * even  the  implied  warranty  of MERCHANTABILITY or
 * FITNESS FOR A PARTICULAR PURPOSE.
 *
 * See   the  GNU  General  Public  License  for  more
 * details. You should have received a copy of the GNU
 * General Public License along with this program.  If
 * not, see <http://www.gnu.org/licenses/>.
 **/
#include <aio4c/acceptor.h>
#include <aio4c/connection.h>
#include <aio4c/log.h>
#include <aio4c/thread.h>

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

static Thread* _main = NULL;
static Acceptor* _acceptor = NULL;

int main(void) {
    char buffer[32];
    ssize_t nbRead = 0;

    _main = ThreadMain("server-main");
    LogInit(_main, DEBUG, "server.log");
    Connection* factory = NewConnectionFactory(8192);
    _acceptor = NewAcceptor("server-acceptor", IPV4, "localhost", 11111, factory);

    while ((nbRead = read(STDIN_FILENO, buffer, 31)) > 0) {
        buffer[nbRead] = '\0';
        if (memcmp(buffer, "QUIT\n", 6) == 0) {
            printf("EXITING\n");
            break;
        } else {
            printf("ERROR\n");
        }
    }

    AcceptorEnd(_acceptor);
    LogEnd();
    FreeThread(&_main);

    exit(EXIT_SUCCESS);
}
