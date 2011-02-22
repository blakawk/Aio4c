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
    Connection* connection = NULL;

    while (Dequeue(writer->queue, &item, false)) {
        switch(item->type) {
            case EXIT:
                FreeItem(&item);
                return false;
            case DATA:
                connection = (Connection*)item->content.data;
                Log(writer->thread, DEBUG, "close received for connection %s", connection->string);
                if (connection->writeKey != NULL) {
                    event = (WriterEvent*)connection->writeKey->attachment;
                    FreeBuffer(&event->buffer);
                    free(event);
                    Unregister(writer->selector, connection->writeKey);
                }
                if (ConnectionNoMoreUsed(connection, WRITER)) {
                    Log(writer->thread, DEBUG, "freeing connection %s", connection->string);
                    FreeConnection(&connection);
                }
                break;
            case EVENT:
                event = (WriterEvent*)item->content.event.source;
                event->connection->writeKey = Register(writer->selector, AIO4C_OP_WRITE, event->connection->socket, (void*)event);
                break;
        }
        FreeItem(&item);
    }

    numConnectionsReady = Select(writer->selector);

    if (numConnectionsReady > 0) {
        while (SelectionKeyReady(writer->selector, &key)) {
            connection = ((WriterEvent*)key->attachment)->connection;
            if (key->result == (int)key->operation) {
                event = (WriterEvent*)key->attachment;
                switch (event->type) {
                    case CONNECTING_EVENT:
                        Log(writer->thread, DEBUG, "finish connect for %s", event->connection->string);
                        connection = ConnectionFinishConnect(event->connection);
                        FreeBuffer(&event->buffer);
                        free(event);
                        break;
                    case OUTBOUND_DATA_EVENT:
                        Log(writer->thread, DEBUG, "about to write to %s", event->connection->string);
                        connection = ConnectionWrite(event->connection, event->buffer);

                        if (BufferHasRemaining(event->buffer) && connection->state != CLOSED) {
                            if (!Enqueue(writer->queue, NewEventItem(OUTBOUND_DATA_EVENT, event))) {
                                FreeBuffer(&event->buffer);
                                free(event);
                            }
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

                if (connection != NULL && connection->state == CLOSED) {
                    Log(writer->thread, DEBUG, "connection closed with reason %d, code %d", connection->closeCause, connection->closeCode);
                }
            } else {
                Log(writer->thread, DEBUG, "invalid result %d", key->result);
                if (key->result & POLLOUT) {
                    Log(writer->thread, DEBUG, "write available");
                }

                if (key->result & POLLERR) {
                    Log(writer->thread, DEBUG, "poll error");
                }

                if (key->result & POLLIN) {
                    Log(writer->thread, DEBUG, "read available");
                }

                if (key->result & POLLHUP) {
                    Log(writer->thread, DEBUG, "disconnected");
                }

                if (key->result & POLLNVAL) {
                    Log(writer->thread, DEBUG, "invalid descriptor %d", key->fd);
                }

                event = (WriterEvent*)key->attachment;
                FreeBuffer(&event->buffer);
                free(event);
            }

            Unregister(writer->selector, key);
            connection->writeKey = NULL;
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
    Thread* th = writer->thread;
    QueueItem* item = NewExitItem();

    if (!Enqueue(writer->queue, item)) {
        FreeItem(&item);
        return;
    }

    SelectorWakeUp(writer->selector);

    ThreadJoin(th);
    FreeThread(&th);
}

static void _WriterCloseHandler(Event event, Connection* source, Writer* writer) {
    QueueItem* item = NULL;
    if (event != CLOSE_EVENT) {
        return;
    }

    item = NewDataItem(source);

    if (!Enqueue(writer->queue, item)) {
        FreeItem(&item);
        return;
    }

    SelectorWakeUp(writer->selector);
}

static void _WriterEventHandler(Event event, Connection* source, Writer* writer) {
    WriterEvent* ev = NULL;
    QueueItem* item = NULL;

    if ((ev = malloc(sizeof(WriterEvent))) == NULL) {
        return;
    }

    ev->connection = source;
    ev->buffer = NewBuffer(writer->bufferSize);
    BufferLimit(ev->buffer, 0);
    ev->type = event;

    if (!Enqueue(writer->queue, item = NewEventItem(event, ev))) {
        FreeItem(&item);
        FreeBuffer(&ev->buffer);
        free(ev);
        return;
    }

    SelectorWakeUp(writer->selector);
}

void WriterManageConnection(Writer* writer, Connection* connection) {
    ConnectionAddSystemHandler(connection, CONNECTING_EVENT, aio4c_connection_handler(_WriterEventHandler), aio4c_connection_handler_arg(writer), false);
    ConnectionAddSystemHandler(connection, OUTBOUND_DATA_EVENT, aio4c_connection_handler(_WriterEventHandler), aio4c_connection_handler_arg(writer), false);
    ConnectionAddSystemHandler(connection, CLOSE_EVENT, aio4c_connection_handler(_WriterCloseHandler), aio4c_connection_handler_arg(writer), true);
}

