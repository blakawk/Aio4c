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

static bool _WriterInit(ThreadData _writer) {
    Writer* writer = (Writer*)_writer;
    if ((writer->queue = NewQueue()) == NULL) {
        return false;
    }

    Log(AIO4C_LOG_LEVEL_DEBUG, "initialized with tid 0x%08lx", ThreadGetId(writer->thread));

    return true;
}

static bool _WriterRemove(QueueItem* item, QueueDiscriminant discriminant) {
    switch (QueueItemGetType(item)) {
        case AIO4C_QUEUE_ITEM_EVENT:
            if ((Connection*)QueueEventItemGetSource(item) == (Connection*)discriminant) {
                return true;
            }
            break;
        default:
            break;
    }

    return false;
}

static bool _WriterRun(ThreadData _writer) {
    Writer* writer = (Writer*)_writer;
    QueueItem* item = NewQueueItem();
    Connection* connection = NULL;
    Event event = AIO4C_INIT_EVENT;

    while (Dequeue(writer->queue, item, true)) {
        Log(AIO4C_LOG_LEVEL_DEBUG, "dequeued item %d", QueueItemGetType(item));
        switch(QueueItemGetType(item)) {
            case AIO4C_QUEUE_ITEM_EXIT:
                FreeQueueItem(&item);
                return false;
            case AIO4C_QUEUE_ITEM_EVENT:
                event = QueueEventItemGetEvent(item);
                connection = (Connection*)QueueEventItemGetSource(item);
                switch(event) {
                    case AIO4C_OUTBOUND_DATA_EVENT:
                        Log(AIO4C_LOG_LEVEL_DEBUG, "processing write interest for connection %s", connection->string);
                        if (ConnectionWrite(connection)) {
                            Log(AIO4C_LOG_LEVEL_DEBUG, "did not write all data for connection %s, reenqueueing", connection->string);
                            EnqueueEventItem(writer->queue, QueueEventItemGetEvent(item), QueueEventItemGetSource(item));
                        }
                        break;
                    case AIO4C_CLOSE_EVENT:
                        RemoveAll(writer->queue, _WriterRemove, (QueueDiscriminant)connection);
                        Log(AIO4C_LOG_LEVEL_DEBUG, "close received for connection %s", connection->string);
                        if (ConnectionNoMoreUsed(connection, AIO4C_CONNECTION_OWNER_WRITER)) {
                            Log(AIO4C_LOG_LEVEL_DEBUG, "freeing connection %s", connection->string);
                            FreeConnection(&connection);
                        }
                        break;
                    default:
                        Log(AIO4C_LOG_LEVEL_WARN, "received unexpected event %d", event);
                        break;
                }
                break;
            default:
                break;
        }
    }

    FreeQueueItem(&item);

    return true;
}

static void _WriterExit(ThreadData _writer) {
    Writer* writer = (Writer*)_writer;
    FreeQueue(&writer->queue);

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

    writer->queue        = NULL;
    writer->bufferSize   = bufferSize;
    if (pipeName != NULL) {
        writer->name         = aio4c_malloc(strlen(pipeName) + 1 + 7);
        if (writer->name != NULL) {
            snprintf(writer->name, strlen(pipeName) + 1 + 7, "%s-writer", pipeName);
        }
    } else {
        writer->name = NULL;
    }
    writer->thread       = NewThread(
            writer->name,
            _WriterInit,
            _WriterRun,
            _WriterExit,
            (ThreadData)writer);

    if (writer->thread == NULL) {
        if (writer->name != NULL) {
            aio4c_free(writer->name);
        }
        aio4c_free(writer);
        return NULL;
    }

    if (!ThreadStart(writer->thread)) {
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

        ThreadJoin(writer->thread);
    }

    if (writer->name != NULL) {
        aio4c_free(writer->name);
    }

    aio4c_free(writer);
}

static void _WriterEventHandler(Event event, Connection* source, Writer* writer) {
    Log(AIO4C_LOG_LEVEL_DEBUG, "sending event %d for connection %s to writer %s", event, source->string, ThreadGetName(writer->thread));
    if (!EnqueueEventItem(writer->queue, event, (EventSource)source)) {
        Log(AIO4C_LOG_LEVEL_WARN, "event %d for connection %s lost", event, source->string);
        return;
    }
}

void WriterManageConnection(Writer* writer, Connection* connection) {
    ConnectionAddSystemHandler(connection, AIO4C_OUTBOUND_DATA_EVENT, aio4c_connection_handler(_WriterEventHandler), aio4c_connection_handler_arg(writer), false);
    ConnectionAddSystemHandler(connection, AIO4C_CLOSE_EVENT, aio4c_connection_handler(_WriterEventHandler), aio4c_connection_handler_arg(writer), true);
    ConnectionManagedBy(connection, AIO4C_CONNECTION_OWNER_WRITER);
}
