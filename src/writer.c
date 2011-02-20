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
#include <aio4c/writer.h>

#include <aio4c/buffer.h>
#include <aio4c/connection.h>
#include <aio4c/event.h>
#include <aio4c/log.h>
#include <aio4c/thread.h>

#include <errno.h>
#include <fcntl.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>

static void _WriterInit(Writer* writer) {
    Log(writer->thread, INFO, "initialized");
}

static aio4c_bool_t _WriterRun(Writer* writer) {
    QueueItem* item = NULL;
    WriterEvent* event = NULL;
    SelectionKey* key = NULL;
    int numConnectionsReady = 0;

    while (Dequeue(writer->queue, &item, false)) {
        switch(item->type) {
            case EXIT:
                FreeItem(&item);
                return false;
            case DATA:
                break;
            case EVENT:
                event = (WriterEvent*)item->content.event.source;
                Register(writer->selector, AIO4C_OP_WRITE, event->connection->socket, (void*)event);
                break;
        }
        FreeItem(&item);
    }

    numConnectionsReady = Select(writer->selector);

    if (numConnectionsReady > 0) {
        while (SelectionKeyReady(writer->selector, &key)) {
            if (key->result == key->operation) {
                event = (WriterEvent*)key->attachment;
                switch (event->type) {
                    case CONNECTING_EVENT:
                        Log(writer->thread, DEBUG, "finish connect for %s", event->connection->string);
                        ConnectionFinishConnect(event->connection);
                        FreeBuffer(&event->buffer);
                        free(event);
                        break;
                    case OUTBOUND_DATA_EVENT:
                        Log(writer->thread, DEBUG, "about to write to %s", event->connection->string);
                        ConnectionWrite(event->connection, event->buffer);

                        if (BufferHasRemaining(event->buffer)) {
                            Enqueue(writer->queue, NewEventItem(OUTBOUND_DATA_EVENT, event));
                        } else {
                            FreeBuffer(&event->buffer);
                            free(event);
                        }
                        break;
                   default:
                        Log(writer->thread, DEBUG, "unknown event %d", event->type);
                        FreeBuffer(&event->buffer);
                        free(event);
                        break;
                }
            } else {
                Log(writer->thread, DEBUG, "invalid result %d", key->result);
                switch (key->result) {
                    case POLLOUT:
                        Log(writer->thread, DEBUG, "write available");
                        break;
                    case POLLERR:
                        Log(writer->thread, DEBUG, "poll error");
                        break;
                    case POLLIN:
                        Log(writer->thread, DEBUG, "read available");
                        break;
                    case POLLHUP:
                        Log(writer->thread, DEBUG, "disconnected");
                        break;
                    case POLLNVAL:
                        Log(writer->thread, DEBUG, "invalid descriptor %d", key->fd);
                        break;
                }
            }
            Unregister(writer->selector, key);
        }
    }

    return true;
}

static void _WriterExit(Writer* writer) {
    FreeSelector(&writer->selector);
    FreeQueue(&writer->queue);

    Log(writer->thread, INFO, "exited");

    free(writer);
}

Writer* NewWriter(Thread* parent, char* name, aio4c_size_t bufferSize) {
    Writer* writer = NULL;

    if ((writer = malloc(sizeof(Writer))) == NULL) {
        Log(parent, ERROR, "cannot allocate writer: %s", strerror(errno));
        return NULL;
    }

    writer->selector = NewSelector();
    writer->queue = NewQueue();
    writer->bufferSize = bufferSize;

    writer->thread = NULL;
    writer->thread = NewThread(name, aio4c_thread_handler(_WriterInit), aio4c_thread_run(_WriterRun), aio4c_thread_handler(_WriterExit), aio4c_thread_arg(writer));

    return writer;
}

void WriterEnd(Writer* writer) {
    Enqueue(writer->queue, NewExitItem());

    SelectorWakeUp(writer->selector);
}

static void _WriterEventHandler(Event event, Connection* source, Writer* writer) {
    WriterEvent* ev = NULL;

    if ((ev = malloc(sizeof(WriterEvent))) == NULL) {
        return;
    }

    ev->connection = source;
    ev->buffer = NewBuffer(writer->bufferSize);
    BufferLimit(ev->buffer, 0);
    ev->type = event;

    Enqueue(writer->queue, NewEventItem(event, ev));

    SelectorWakeUp(writer->selector);
}

void WriterManageConnection(Writer* writer, Connection* connection) {
    ConnectionAddSystemHandler(connection, CONNECTING_EVENT, aio4c_connection_handler(_WriterEventHandler), aio4c_connection_handler_arg(writer), false);
    ConnectionAddSystemHandler(connection, OUTBOUND_DATA_EVENT, aio4c_connection_handler(_WriterEventHandler), aio4c_connection_handler_arg(writer), false);
}

