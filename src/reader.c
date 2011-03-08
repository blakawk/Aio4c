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
#include <aio4c/reader.h>

#include <aio4c/alloc.h>
#include <aio4c/connection.h>
#include <aio4c/log.h>
#include <aio4c/stats.h>
#include <aio4c/thread.h>
#include <aio4c/types.h>

#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#ifndef AIO4C_WIN32
#include <errno.h>
#include <fcntl.h>
#include <poll.h>
#else
#include <winsock2.h>
#endif

static void _ReaderInit(Reader* reader) {
    Log(reader->thread, AIO4C_LOG_LEVEL_INFO, "initialized");
}

static aio4c_bool_t _ReaderRun(Reader* reader) {
    QueueItem item;
    Connection* connection = NULL;
    SelectionKey* key = NULL;
    int numConnectionsReady = 0;

    while(Dequeue(reader->queue, &item, false)) {
        switch(item.type) {
            case AIO4C_QUEUE_ITEM_EXIT:
                return false;
            case AIO4C_QUEUE_ITEM_DATA:
                connection = (Connection*)item.content.data;
                connection->readKey = Register(reader->selector, AIO4C_OP_READ, connection->socket, (void*)connection);
                Log(reader->thread, AIO4C_LOG_LEVEL_DEBUG, "managing connection %s", connection->string);
                break;
            case AIO4C_QUEUE_ITEM_EVENT:
                connection = (Connection*)item.content.event.source;
                if (connection->readKey != NULL) {
                    Unregister(reader->selector, connection->readKey, true);
                    connection->readKey = NULL;
                }
                Log(reader->thread, AIO4C_LOG_LEVEL_DEBUG, "close received for connection %s", connection->string);
                if (ConnectionNoMoreUsed(connection, AIO4C_CONNECTION_OWNER_READER)) {
                    Log(reader->thread, AIO4C_LOG_LEVEL_DEBUG, "freeing connection %s", connection->string);
                    FreeConnection(&connection);
                }
                break;
            default:
                break;
        }
    }

    ProbeTimeStart(AIO4C_TIME_PROBE_IDLE);
    numConnectionsReady = Select(reader->selector);
    ProbeTimeEnd(AIO4C_TIME_PROBE_IDLE);

    if (numConnectionsReady > 0) {
        ProbeTimeStart(AIO4C_TIME_PROBE_NETWORK_READ);
        while (SelectionKeyReady(reader->selector, &key)) {
            if (key->result & (int)key->operation) {
                connection = ConnectionRead((Connection*)key->attachment);
            } else {
                ConnectionClose((Connection*)key->attachment);
            }
        }
        ProbeTimeEnd(AIO4C_TIME_PROBE_NETWORK_READ);
    }

    return true;
}

static void _ReaderExit(Reader* reader) {
    WorkerEnd(reader->worker);

    FreeQueue(&reader->queue);
    FreeSelector(&reader->selector);

    Log(reader->thread, AIO4C_LOG_LEVEL_INFO, "exited");

    aio4c_free(reader);
}

Reader* NewReader(aio4c_size_t bufferSize) {
    Reader* reader = NULL;

    if ((reader = aio4c_malloc(sizeof(Reader))) == NULL) {
        Log(ThreadSelf(), AIO4C_LOG_LEVEL_ERROR, "reader allocation: %s", strerror(errno));
        return NULL;
    }

    reader->selector = NewSelector();
    reader->queue = NewQueue();
    reader->worker = NULL;
    reader->worker = NewWorker(bufferSize);
    reader->thread = NULL;
    reader->thread = NewThread("reader", aio4c_thread_handler(_ReaderInit), aio4c_thread_run(_ReaderRun), aio4c_thread_handler(_ReaderExit), aio4c_thread_arg(reader));

    return reader;
}

static void _ReaderEventHandler(Event event, Connection* connection, Reader* reader) {
    if (!EnqueueEventItem(reader->queue, event, connection)) {
        return;
    }

    SelectorWakeUp(reader->selector);
}

void ReaderManageConnection(Reader* reader, Connection* connection) {
    if (!EnqueueDataItem(reader->queue, connection)) {
        Log(NULL, AIO4C_LOG_LEVEL_WARN, "reader will not manage connection %s", connection->string);
        return;
    }

    ConnectionAddSystemHandler(connection, AIO4C_CLOSE_EVENT, aio4c_connection_handler(_ReaderEventHandler), aio4c_connection_handler_arg(reader), true);

    WorkerManageConnection(reader->worker, connection);

    SelectorWakeUp(reader->selector);
}

void ReaderEnd(Reader* reader) {
    Thread* th = reader->thread;

    if (!EnqueueExitItem(reader->queue)) {
        return;
    }

    SelectorWakeUp(reader->selector);

    ThreadJoin(th);

    FreeThread(&th);
}

