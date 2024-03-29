/**
 * Copyright (c) 2011 blakawk
 *
 * This file is part of Aio4c <http://aio4c.so>.
 *
 * Aio4c <http://aio4c.so> is free software: you
 * can  redistribute  it  and/or modify it under
 * the  terms  of the GNU General Public License
 * as published by the Free Software Foundation,
 * version 3 of the License.
 *
 * Aio4c <http://aio4c.so> is distributed in the
 * hope  that it will be useful, but WITHOUT ANY
 * WARRANTY;  without  even the implied warranty
 * of   MERCHANTABILITY   or   FITNESS   FOR   A
 * PARTICULAR PURPOSE.
 *
 * See  the  GNU General Public License for more
 * details.  You  should have received a copy of
 * the  GNU  General  Public  License along with
 * Aio4c    <http://aio4c.so>.   If   not,   see
 * <http://www.gnu.org/licenses/>.
 */
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
    aio4c_byte_t* bdata = NULL;

    switch (event) {
        case AIO4C_READ_EVENT:
            buffer = source->dataBuffer;
            bdata = BufferGetBytes(buffer);
            LogBuffer(AIO4C_LOG_LEVEL_DEBUG, buffer);
            if (memcmp(&bdata[BufferGetPosition(buffer)], "PING ", 6) == 0) {
                BufferPosition(buffer, BufferGetPosition(buffer) + 6);
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
                    ConnectionClose(source, true);
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
            BufferFlip(buffer);
            LogBuffer(AIO4C_LOG_LEVEL_DEBUG, buffer);
            BufferPosition(buffer, BufferGetLimit(buffer));
            BufferLimit(buffer, BufferGetCapacity(buffer));
            break;
        case AIO4C_FREE_EVENT:
            free(data);
            break;
        default:
            break;
    }
}

void* dataFactory(Connection* c, void* arg __attribute__((unused))) {
    Log(AIO4C_LOG_LEVEL_DEBUG, "creating data for connection %s", c->string);
    Data* data = malloc(sizeof(Data));
    memset(data, 0, sizeof(Data));
    data->lastSeq = 0;
    return data;
}

int main(int argc, char* argv[]) {
    char buffer[32];
    ssize_t nbRead = 0;

    Aio4cInit(argc, argv, NULL, NULL);

    Server* server = NewServer(AIO4C_ADDRESS_IPV4, "localhost", 11111, 8192, 1, aio4c_server_handler(serverHandler), NULL, dataFactory);
    if (server != NULL && ServerStart(server)) {
        while ((nbRead = read(STDIN_FILENO, buffer, 31)) > 0) {
            buffer[nbRead] = '\0';
            if (memcmp(buffer, "QUIT\n", 6) == 0) {
                printf("EXITING\n");
                break;
            } else {
                printf("ERROR\n");
            }
        }

        ServerStop(server);
    }

    ServerJoin(server);
    Aio4cEnd();

    exit(EXIT_SUCCESS);
}
