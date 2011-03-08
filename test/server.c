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

typedef struct s_Data {
    struct timeval ping;
    int lastSeq;
} Data;

static void serverHandler(Event event, Connection* source, Data* data) {
    Buffer* buffer = NULL;
    int curSeq = 0;
    struct timeval pong;

    switch (event) {
        case AIO4C_INBOUND_DATA_EVENT:
            buffer = source->dataBuffer;
            if (memcmp(&buffer->data[buffer->position], "PING ", 6) == 0) {
                buffer->position += 6;
                memcpy(&data->ping, &buffer->data[buffer->position], sizeof(struct timeval));
                buffer->position += sizeof(struct timeval);
                memcpy(&curSeq, &buffer->data[buffer->position], sizeof(int));
                buffer->position += sizeof(int);
                if (curSeq == data->lastSeq + 1) {
                    data->lastSeq ++;
                    EnableWriteInterest(source);
                }
            }
            break;
        case AIO4C_WRITE_EVENT:
            buffer = source->writeBuffer;
            memcpy(&buffer->data[buffer->position], "PONG ", 6);
            buffer->position += 6;
            gettimeofday(&pong, NULL);
            memcpy(&buffer->data[buffer->position], &data->ping, sizeof(struct timeval));
            buffer->position += sizeof(struct timeval);
            memcpy(&buffer->data[buffer->position], &pong, sizeof(struct timeval));
            buffer->position += sizeof(struct timeval);
            memcpy(&buffer->data[buffer->position], &data->lastSeq, sizeof(int));
            buffer->position += sizeof(int);
            break;
        case AIO4C_CLOSE_EVENT:
            free(data);
            break;
        default:
            break;
    }
}

void* dataFactory(Connection* c) {
    Log(NULL, AIO4C_LOG_LEVEL_DEBUG, "creating data for connection %s", c->string);
    return malloc(sizeof(Data));
}

int main(void) {
    char buffer[32];
    ssize_t nbRead = 0;

    Server* server = NewServer(AIO4C_ADDRESS_IPV4, "localhost", 11111, AIO4C_LOG_LEVEL_DEBUG, "server.log", 8192, aio4c_server_handler(serverHandler), dataFactory);

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
