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
#include <aio4c/buffer.h>
#include <aio4c/client.h>
#include <aio4c/connection.h>
#include <aio4c/event.h>
#include <aio4c/log.h>
#include <aio4c/thread.h>
#include <aio4c/types.h>
#include <aio4c/stats.h>

#include <stdlib.h>
#include <string.h>
#include <sys/time.h>

static int seq = 0;

void onRead(Connection* source) {
    Buffer* buffer = source->dataBuffer;
    struct timeval ping, pong;
    int mySeq = 0;

    if (memcmp(&buffer->data[buffer->position], "PONG ", 6) == 0) {
        buffer->position += 6;
        memcpy(&ping, &buffer->data[buffer->position], sizeof(struct timeval));
        buffer->position += sizeof(struct timeval);
        memcpy(&pong, &buffer->data[buffer->position], sizeof(struct timeval));
        buffer->position += sizeof(struct timeval);
        memcpy(&mySeq, &buffer->data[buffer->position], sizeof(int));
        buffer->position += sizeof(int);
//        Log(NULL, INFO, "pong received, seq = %d, time = %.3f ms", mySeq,
 //               (pong.tv_sec - ping.tv_sec) * 1000.0 + (pong.tv_usec - ping.tv_usec) / 1000.0);
        ProbeSize(PROBE_LATENCY_COUNT, 1);
        ProbeTime(TIME_PROBE_LATENCY, &ping, &pong);
    }

    ConnectionEventHandle(source, OUTBOUND_DATA_EVENT, source);
}

void onWrite(Connection* source) {
    Buffer* buffer = source->writeBuffer;
    struct timeval ping;

    seq ++;

    gettimeofday(&ping, NULL);
    memcpy(&buffer->data[buffer->position], "PING ", 6);
    buffer->position += 6;
    memcpy(&buffer->data[buffer->position], &ping, sizeof(struct timeval));
    buffer->position += sizeof(struct timeval);
    memcpy(&buffer->data[buffer->position], &seq, sizeof(int));
    buffer->position += sizeof(int);
}

void handler(Event event, Connection* source, void* c) {
    if (c != NULL) {
        return;
    }

    switch (event) {
        case INBOUND_DATA_EVENT:
            onRead(source);
            break;
        case WRITE_EVENT:
            onWrite(source);
            break;
        case CONNECTED_EVENT:
            ConnectionEventHandle(source, OUTBOUND_DATA_EVENT, source);
            break;
        default:
            Log(NULL, DEBUG, "received event %d", event);
            break;
    }
}

int main (void) {
    Thread* client = NULL;
    Thread* tMain = NULL;

    LogInit(tMain = ThreadMain("main"), INFO, NULL);

    client = NewClient(IPV4, "localhost", 11111, 3, 10, 8192, handler, NULL);
    ThreadJoin(client);
    FreeThread(&client);
    LogEnd();
    FreeThread(&tMain);

    return EXIT_SUCCESS;
}

