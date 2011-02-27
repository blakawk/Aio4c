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

#include <stdlib.h>
#include <string.h>

void onRead(Event event, Connection* source, Client* c) {
    if (event != INBOUND_DATA_EVENT) {
        return;
    }

    Log(c->thread, DEBUG, "read %d bytes", source->dataBuffer->limit);
    LogBuffer(c->thread, DEBUG, source->dataBuffer);
    source->dataBuffer->position = source->dataBuffer->limit;
    ConnectionEventHandle(source, OUTBOUND_DATA_EVENT, source);
}

void onWrite(Event event, Connection* source, Client* c) {
    if (event != WRITE_EVENT) {
        return;
    }

    memcpy(source->writeBuffer->data, "HELLO\n", 7);
    source->writeBuffer->limit = 7;
    Log(c->thread, DEBUG, "wrote %d bytes", 7);
    LogBuffer(c->thread, DEBUG, source->writeBuffer);
    source->writeBuffer->position += 7;
}

int main (void) {
    Thread* client = NULL;
    Thread* tMain = NULL;

    LogInit(tMain = ThreadMain("main"), INFO, "client.log");

    client = NewClient(IPV4, "localhost", 11111, 3, 10, 8192, onRead, onWrite);
    ThreadJoin(client);
    FreeThread(&client);
    LogEnd();
    FreeThread(&tMain);

    return EXIT_SUCCESS;
}

