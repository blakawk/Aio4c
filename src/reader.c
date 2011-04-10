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
#include <aio4c/error.h>
#include <aio4c/log.h>
#include <aio4c/stats.h>
#include <aio4c/thread.h>
#include <aio4c/types.h>
#include <aio4c/worker.h>

#ifndef AIO4C_WIN32

#include <errno.h>

#endif /* AIO4C_WIN32 */

#include <stdlib.h>
#include <string.h>

static aio4c_bool_t _ReaderInit(Reader* reader) {
    if ((reader->selector = NewSelector()) == NULL) {
        return false;
    }

    if ((reader->queue = NewQueue()) == NULL) {
        return false;
    }

    if ((reader->worker = NewWorker(reader->pipe, reader->bufferSize)) == NULL) {
        return false;
    }

    Log(AIO4C_LOG_LEVEL_DEBUG, "initialized with tid 0x%08lx", reader->thread->id);

    return true;
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
                Log(AIO4C_LOG_LEVEL_DEBUG, "managing connection %s", connection->string);
                ConnectionManagedBy(connection, AIO4C_CONNECTION_OWNER_READER);
                reader->load++;
                break;
            case AIO4C_QUEUE_ITEM_EVENT:
                connection = (Connection*)item.content.event.source;
                if (item.content.event.type == AIO4C_CLOSE_EVENT) {
                    if (connection->readKey != NULL) {
                        Unregister(reader->selector, connection->readKey, true);
                        connection->readKey = NULL;
                    }
                    Log(AIO4C_LOG_LEVEL_DEBUG, "close received for connection %s", connection->string);
                    reader->load--;
                    if (ConnectionNoMoreUsed(connection, AIO4C_CONNECTION_OWNER_READER)) {
                        Log(AIO4C_LOG_LEVEL_DEBUG, "freeing connection %s", connection->string);
                        FreeConnection(&connection);
                    }
                } else if (item.content.event.type == AIO4C_PENDING_CLOSE_EVENT) {
                    Log(AIO4C_LOG_LEVEL_DEBUG, "pending close received for connection %s", connection->string);
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
            }
        }
        ProbeTimeEnd(AIO4C_TIME_PROBE_NETWORK_READ);
    }

    return true;
}

static void _ReaderExit(Reader* reader) {
    if (reader->worker != NULL) {
        WorkerEnd(reader->worker);
    }

    FreeQueue(&reader->queue);

    Log(AIO4C_LOG_LEVEL_DEBUG, "exited");
}

Reader* NewReader(char* pipeName, aio4c_size_t bufferSize) {
    Reader* reader = NULL;
    ErrorCode code = AIO4C_ERROR_CODE_INITIALIZER;

    if ((reader = aio4c_malloc(sizeof(Reader))) == NULL) {
#ifndef AIO4C_WIN32
        code.error = errno;
#else /* AIO4C_WIN32 */
        code.source = AIO4C_ERRNO_SOURCE_SYS;
#endif /* AIO4C_WIN32 */
        code.size = sizeof(Reader);
        code.type = "Reader";
        Raise(AIO4C_LOG_LEVEL_ERROR, AIO4C_ALLOC_ERROR_TYPE, AIO4C_ALLOC_ERROR, &code);
        return NULL;
    }

    reader->selector   = NULL;
    reader->queue      = NULL;
    reader->worker     = NULL;
    reader->bufferSize = bufferSize;
    reader->load       = 0;

    if (pipeName != NULL) {
        reader->pipe       = pipeName;
        reader->name       = aio4c_malloc(strlen(pipeName) + 1 + 7);
        if (reader->name != NULL) {
            snprintf(reader->name, strlen(pipeName) + 1 + 7, "%s-reader", pipeName);
        }
    } else {
        reader->pipe = NULL;
        reader->name = NULL;
    }

    reader->thread     = NULL;

    NewThread(reader->name,
           aio4c_thread_init(_ReaderInit),
           aio4c_thread_run(_ReaderRun),
           aio4c_thread_exit(_ReaderExit),
           aio4c_thread_arg(reader),
           &reader->thread);

    if (reader->thread == NULL) {
        FreeSelector(&reader->selector);
        if (reader->pipe != NULL) {
            aio4c_free(reader->pipe);
        }
        if (reader->name != NULL) {
            aio4c_free(reader->name);
        }
        aio4c_free(reader);
        return NULL;
    }

    return reader;
}

static void _ReaderEventHandler(Event event, Connection* connection, Reader* reader) {
    if (reader->queue == NULL || !EnqueueEventItem(reader->queue, event, connection)) {
        if (ConnectionNoMoreUsed(connection, AIO4C_CONNECTION_OWNER_READER)) {
            FreeConnection(&connection);
        }
        return;
    }

    SelectorWakeUp(reader->selector);
}

void ReaderManageConnection(Reader* reader, Connection* connection) {
    if (!EnqueueDataItem(reader->queue, connection)) {
        Log(AIO4C_LOG_LEVEL_WARN, "reader will not manage connection %s", connection->string);
        return;
    }

    ConnectionAddSystemHandler(connection, AIO4C_PENDING_CLOSE_EVENT, aio4c_connection_handler(_ReaderEventHandler), aio4c_connection_handler_arg(reader), true);
    ConnectionAddSystemHandler(connection, AIO4C_CLOSE_EVENT, aio4c_connection_handler(_ReaderEventHandler), aio4c_connection_handler_arg(reader), true);

    WorkerManageConnection(reader->worker, connection);

    SelectorWakeUp(reader->selector);
}

void ReaderEnd(Reader* reader) {
    if (reader->thread != NULL) {
        if (reader->queue != NULL) {
            EnqueueExitItem(reader->queue);
        }

        if (reader->selector != NULL) {
            SelectorWakeUp(reader->selector);
        }

        ThreadJoin(reader->thread);
    }

    FreeSelector(&reader->selector);

    if (reader->name != NULL) {
        aio4c_free(reader->name);
    }

    if (reader->pipe != NULL) {
        aio4c_free(reader->pipe);
    }

    aio4c_free(reader);
}

