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
#include <aio4c/error.h>
#include <aio4c/event.h>
#include <aio4c/log.h>
#include <aio4c/stats.h>
#include <aio4c/thread.h>

#ifndef AIO4C_WIN32
#include <errno.h>
#include <sys/socket.h>
#else /* AIO4C_WIN32 */
#include <winsock2.h>
#endif /* AIO4C_WIN32 */

#include <string.h>

static aio4c_bool_t _WriterInit(Writer* writer) {
    if ((writer->selector = NewSelector()) == NULL) {
        return false;
    }

    if ((writer->queue = NewQueue()) == NULL) {
        return false;
    }

    if ((writer->toUnregister = NewQueue()) == NULL) {
        return false;
    }

    Log(AIO4C_LOG_LEVEL_DEBUG, "initialized with tid 0x%08lx", writer->thread->id);

    return true;
}

static aio4c_bool_t _WriterRemove(QueueItem* item, Connection* discriminant) {
    switch (item->type) {
        case AIO4C_QUEUE_ITEM_EVENT:
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
            case AIO4C_QUEUE_ITEM_EXIT:
                return false;
            case AIO4C_QUEUE_ITEM_DATA:
                connection = (Connection*)item.content.data;
                if (connection->state == AIO4C_CONNECTION_STATE_CLOSED) {
                    if (connection->writeKey != NULL) {
                        Unregister(writer->selector, connection->writeKey, true);
                    }
                    RemoveAll(writer->queue, aio4c_remove_callback(_WriterRemove), aio4c_remove_discriminant(connection));
                    Log(AIO4C_LOG_LEVEL_DEBUG, "close received for connection %s", connection->string);
                    if (ConnectionNoMoreUsed(connection, AIO4C_CONNECTION_OWNER_WRITER)) {
                        Log(AIO4C_LOG_LEVEL_DEBUG, "freeing connection %s", connection->string);
                        FreeConnection(&connection);
                    }
                } else if (connection->state == AIO4C_CONNECTION_STATE_PENDING_CLOSE) {
                    Log(AIO4C_LOG_LEVEL_DEBUG, "pending close received for connection %s", connection->string);
                    if (connection->writeKey == NULL) {
                        connection->writeKey = Register(writer->selector, AIO4C_OP_WRITE, connection->socket, connection);
                    } else {
                        Unregister(writer->selector, connection->writeKey, true);
                        connection->writeKey = Register(writer->selector, AIO4C_OP_WRITE, connection->socket, connection);
                    }
                }
                break;
            case AIO4C_QUEUE_ITEM_EVENT:
                ProbeTimeStart(AIOC_TIME_PROBE_NETWORK_WRITE);
                connection = (Connection*)item.content.event.source;
                Log(AIO4C_LOG_LEVEL_DEBUG, "processing write interest for connection %s", connection->string);
                if (connection->state == AIO4C_CONNECTION_STATE_CONNECTED) {
                    connection->writeKey = Register(writer->selector, AIO4C_OP_WRITE, connection->socket, connection);
                }
                ProbeTimeEnd(AIO4C_TIME_PROBE_NETWORK_WRITE);
                break;
            default:
                break;
        }
    }

    ProbeTimeStart(AIO4C_TIME_PROBE_IDLE);
    numConnectionsReady = Select(writer->selector);
    ProbeTimeEnd(AIO4C_TIME_PROBE_IDLE);

    if (numConnectionsReady > 0) {
        ProbeTimeStart(AIO4C_TIME_PROBE_NETWORK_WRITE);

        while (SelectionKeyReady(writer->selector, &key)) {
            connection = (Connection*)key->attachment;
            unregister = true;

            if (key->result & key->operation) {
                if (ConnectionWrite(connection)) {
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

        ProbeTimeEnd(AIO4C_TIME_PROBE_NETWORK_WRITE);
    }

    return true;
}

static void _WriterExit(Writer* writer) {
    FreeQueue(&writer->queue);
    FreeQueue(&writer->toUnregister);

    Log(AIO4C_LOG_LEVEL_DEBUG, "exited");
}

Writer* NewWriter(char* pipeName, aio4c_size_t bufferSize) {
    Writer* writer = NULL;
    ErrorCode code = AIO4C_ERROR_CODE_INITIALIZER;

    if ((writer = aio4c_malloc(sizeof(Writer))) == NULL) {
#ifndef AIO4C_WIN32
        code.error = errno;
#else /* AIO4C_WIN32 */
        code.source = AIO4C_ERRNO_SOURCE_SYS;
#endif /* AIO4C_WIN32 */
        code.size = sizeof(Writer);
        code.type = "Writer";
        Raise(AIO4C_LOG_LEVEL_ERROR, AIO4C_ALLOC_ERROR_TYPE, AIO4C_ALLOC_ERROR, &code);
        return NULL;
    }

    writer->selector     = NULL;
    writer->queue        = NULL;
    writer->toUnregister = NULL;
    writer->bufferSize   = bufferSize;
    if (pipeName != NULL) {
        writer->name         = aio4c_malloc(strlen(pipeName) + 1 + 7);
        if (writer->name != NULL) {
            snprintf(writer->name, strlen(pipeName) + 1 + 7, "%s-writer", pipeName);
        }
    } else {
        writer->name = NULL;
    }
    writer->thread       = NULL;

    NewThread(writer->name,
           aio4c_thread_init(_WriterInit),
           aio4c_thread_run(_WriterRun),
           aio4c_thread_exit(_WriterExit),
           aio4c_thread_arg(writer),
           &writer->thread);

    if (writer->thread == NULL) {
        FreeSelector(&writer->selector);
        if (writer->name != NULL) {
            aio4c_free(writer->name);
        }
        aio4c_free(writer);
        return NULL;
    }

    return writer;
}

void WriterEnd(Writer* writer) {
    if (writer->thread != NULL) {
        if (writer->queue != NULL) {
            EnqueueExitItem(writer->queue);
        }

        if (writer->selector != NULL) {
            SelectorWakeUp(writer->selector);
        }

        ThreadJoin(writer->thread);
    }

    FreeSelector(&writer->selector);

    if (writer->name != NULL) {
        aio4c_free(writer->name);
    }

    aio4c_free(writer);
}

static void _WriterCloseHandler(Event event, Connection* source, Writer* writer) {
    if (event != AIO4C_CLOSE_EVENT && event != AIO4C_PENDING_CLOSE_EVENT) {
        return;
    }

    if (writer->queue == NULL || !EnqueueDataItem(writer->queue, source)) {
        if (ConnectionNoMoreUsed(source, AIO4C_CONNECTION_OWNER_WRITER)) {
            FreeConnection(&source);
        }
        return;
    }

    SelectorWakeUp(writer->selector);
}

static void _WriterEventHandler(Event event, Connection* source, Writer* writer) {
    if (source->state != AIO4C_CONNECTION_STATE_CLOSED) {
        if (!EnqueueEventItem(writer->queue, event, source)) {
            Log(AIO4C_LOG_LEVEL_WARN, "event %d for connection %s lost", event, source->string);
            return;
        }

        SelectorWakeUp(writer->selector);
    }
}

void WriterManageConnection(Writer* writer, Connection* connection) {
    ConnectionAddSystemHandler(connection, AIO4C_OUTBOUND_DATA_EVENT, aio4c_connection_handler(_WriterEventHandler), aio4c_connection_handler_arg(writer), false);
    ConnectionAddSystemHandler(connection, AIO4C_PENDING_CLOSE_EVENT, aio4c_connection_handler(_WriterCloseHandler), aio4c_connection_handler_arg(writer), true);
    ConnectionAddSystemHandler(connection, AIO4C_CLOSE_EVENT, aio4c_connection_handler(_WriterCloseHandler), aio4c_connection_handler_arg(writer), true);
    ConnectionManagedBy(connection, AIO4C_CONNECTION_OWNER_WRITER);
}

