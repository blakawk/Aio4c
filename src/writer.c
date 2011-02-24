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

#include <aio4c/alloc.h>
#include <aio4c/buffer.h>
#include <aio4c/connection.h>
#include <aio4c/event.h>
#include <aio4c/log.h>
#include <aio4c/stats.h>
#include <aio4c/thread.h>

#include <errno.h>
#include <fcntl.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>

static void _WriterInit(Writer* writer) {
    Log(writer->thread, INFO, "initialized");
}

static aio4c_bool_t _WriterRemove(QueueItem* item, Connection* discriminant) {
    switch (item->type) {
        case DATA:
            if (item->content.data == discriminant) {
                return true;
            }
            break;
        case EVENT:
            if (item->content.event.source == discriminant) {
                return true;
            }
            break;
        default:
            break;
    }

    return false;
}

static aio4c_bool_t _WriterRun(Writer* writer) {
    QueueItem item;
    SelectionKey* key = NULL;
    int numConnectionsReady = 0;
    Connection* connection = NULL;
    aio4c_bool_t unregister = false;

    memset(&item, 0, sizeof(QueueItem));

    while (Dequeue(writer->queue, &item, false)) {
        switch(item.type) {
            case EXIT:
                return false;
            case DATA:
                connection = (Connection*)item.content.data;
                if (connection->writeKey != NULL) {
                    Unregister(writer->selector, connection->writeKey, true);
                }
                Log(writer->thread, DEBUG, "close received for connection %s", connection->string);
                if (ConnectionNoMoreUsed(connection, WRITER)) {
                    Log(writer->thread, DEBUG, "freeing connection %s", connection->string);
                    FreeConnection(&connection);
                }
                break;
            case EVENT:
                connection = (Connection*)item.content.event.source;
                connection->writeKey = Register(writer->selector, AIO4C_OP_WRITE, connection->socket, connection);
                break;
        }
    }

    ProbeTimeStart(TIME_PROBE_IDLE);
    numConnectionsReady = Select(writer->selector);
    ProbeTimeEnd(TIME_PROBE_IDLE);

    if (numConnectionsReady > 0) {
        while (SelectionKeyReady(writer->selector, &key)) {
            connection = (Connection*)key->attachment;
            unregister = true;

            if (key->result & (int)key->operation) {
                ProbeTimeStart(TIME_PROBE_NETWORK_WRITE);
                ConnectionWrite(connection);
                ProbeTimeEnd(TIME_PROBE_NETWORK_WRITE);

                if (BufferHasRemaining(connection->writeBuffer) && connection->state != CLOSED) {
                    unregister = false;
                }
            }

            if (unregister) {
                EnqueueDataItem(writer->toUnregister, key);
            }
        }

        while (Dequeue(writer->toUnregister, &item, false)) {
            Unregister(writer->selector, (SelectionKey*)item.content.data, false);
        }
    }

    return true;
}

static void _WriterExit(Writer* writer) {
    FreeSelector(&writer->selector);
    FreeQueue(&writer->queue);
    FreeQueue(&writer->toUnregister);

    Log(writer->thread, INFO, "exited");

    aio4c_free(writer);
}

Writer* NewWriter(Thread* parent, char* name, aio4c_size_t bufferSize) {
    Writer* writer = NULL;

    if ((writer = aio4c_malloc(sizeof(Writer))) == NULL) {
        Log(parent, ERROR, "cannot allocate writer: %s", strerror(errno));
        return NULL;
    }

    writer->selector = NewSelector();
    writer->queue = NewQueue();
    writer->toUnregister = NewQueue();
    writer->bufferSize = bufferSize;

    writer->thread = NULL;
    writer->thread = NewThread(name, aio4c_thread_handler(_WriterInit), aio4c_thread_run(_WriterRun), aio4c_thread_handler(_WriterExit), aio4c_thread_arg(writer));

    return writer;
}

void WriterEnd(Writer* writer) {
    Thread* th = writer->thread;

    if (!EnqueueExitItem(writer->queue)) {
        return;
    }

    SelectorWakeUp(writer->selector);

    ThreadJoin(th);
    FreeThread(&th);
}

static void _WriterCloseHandler(Event event, Connection* source, Writer* writer) {
    if (event != CLOSE_EVENT) {
        return;
    }

    RemoveAll(writer->queue, aio4c_remove_callback(_WriterRemove), aio4c_remove_discriminant(source));

    if (!EnqueueDataItem(writer->queue, source)) {
        return;
    }

    SelectorWakeUp(writer->selector);
}

static void _WriterEventHandler(Event event, Connection* source, Writer* writer) {
    if (source->state != CLOSED) {
        if (!EnqueueEventItem(writer->queue, event, source)) {
            return;
        }

        SelectorWakeUp(writer->selector);
    }
}

void WriterManageConnection(Writer* writer, Connection* connection) {
    ConnectionAddSystemHandler(connection, OUTBOUND_DATA_EVENT, aio4c_connection_handler(_WriterEventHandler), aio4c_connection_handler_arg(writer), false);
    ConnectionAddSystemHandler(connection, CLOSE_EVENT, aio4c_connection_handler(_WriterCloseHandler), aio4c_connection_handler_arg(writer), true);
}

