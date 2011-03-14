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
#include <aio4c.h>

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
                BufferGet(buffer, &data->ping, sizeof(struct timeval));
                BufferGetInt(buffer, &curSeq);
                if (curSeq == data->lastSeq + 1) {
                    data->lastSeq ++;
                    EnableWriteInterest(source);
                } else if (data->lastSeq == 0) {
                    data->lastSeq = curSeq;
                    EnableWriteInterest(source);
                } else {
                    Log(AIO4C_LOG_LEVEL_DEBUG, "unexpected sequence %d, expected %d for connection %s", curSeq, data->lastSeq + 1, source->string);
                    ConnectionClose(source);
                }
            }
            break;
        case AIO4C_WRITE_EVENT:
            buffer = source->writeBuffer;
            BufferPutString(buffer, "PONG ");
            gettimeofday(&pong, NULL);
            BufferPut(buffer, &data->ping, sizeof(struct timeval));
            BufferPut(buffer, &pong, sizeof(struct timeval));
            BufferPutInt(buffer, &data->lastSeq);
            break;
        case AIO4C_CLOSE_EVENT:
            free(data);
            break;
        default:
            break;
    }
}

void* dataFactory(Connection* c) {
    Log(AIO4C_LOG_LEVEL_DEBUG, "creating data for connection %s", c->string);
    Data* data = malloc(sizeof(Data));
    memset(data, 0, sizeof(Data));
    data->lastSeq = 0;
    return data;
}

int main(void) {
    char buffer[32];
    ssize_t nbRead = 0;

    Aio4cInit(AIO4C_LOG_LEVEL_DEBUG, "server.log");

    Server* server = NewServer(AIO4C_ADDRESS_IPV4, "localhost", 11111, 8192, 32, aio4c_server_handler(serverHandler), dataFactory);

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
    Aio4cEnd();

    exit(EXIT_SUCCESS);
}
