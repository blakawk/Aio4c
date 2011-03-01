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
#include <aio4c/connection.h>
#include <aio4c/server.h>
#include <aio4c/thread.h>

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/time.h>
#include <unistd.h>

static void serverHandler(Event event, Connection* source, void* null) {
    Buffer* buffer = NULL;
    static struct timeval ping;
    static int lastSeq = 0;
    int curSeq = 0;
    struct timeval pong;

    if (null != NULL) {
        return;
    }

    switch (event) {
        case INBOUND_DATA_EVENT:
            buffer = source->dataBuffer;
            if (memcmp(&buffer->data[buffer->position], "PING ", 6) == 0) {
                buffer->position += 6;
                memcpy(&ping, &buffer->data[buffer->position], sizeof(struct timeval));
                buffer->position += sizeof(struct timeval);
                memcpy(&curSeq, &buffer->data[buffer->position], sizeof(int));
                buffer->position += sizeof(int);
                if (curSeq == lastSeq + 1) {
                    lastSeq ++;
                    ConnectionEventHandle(source, OUTBOUND_DATA_EVENT, source);
                }
            }
            break;
        case WRITE_EVENT:
            buffer = source->writeBuffer;
            memcpy(&buffer->data[buffer->position], "PONG ", 6);
            buffer->position += 6;
            gettimeofday(&pong, NULL);
            memcpy(&buffer->data[buffer->position], &ping, sizeof(struct timeval));
            buffer->position += sizeof(struct timeval);
            memcpy(&buffer->data[buffer->position], &pong, sizeof(struct timeval));
            buffer->position += sizeof(struct timeval);
            memcpy(&buffer->data[buffer->position], &lastSeq, sizeof(int));
            buffer->position += sizeof(int);
            break;
        default:
            break;
    }
}

int main(void) {
    char buffer[32];
    ssize_t nbRead = 0;

    Server* server = NewServer(IPV4, "localhost", 11111, DEBUG, NULL, 8192, serverHandler, NULL);

    while ((nbRead = read(STDIN_FILENO, buffer, 31)) > 0) {
        buffer[nbRead] = '\0';
        if (memcmp(buffer, "QUIT\n", 6) == 0) {
            printf("EXITING\n");
            break;
        } else {
            printf("ERROR\n");
        }
    }

    ServerEnd(server);

    exit(EXIT_SUCCESS);
}
