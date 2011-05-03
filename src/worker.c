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
#include <aio4c/worker.h>

#include <aio4c/alloc.h>
#include <aio4c/buffer.h>
#include <aio4c/connection.h>
#include <aio4c/error.h>
#include <aio4c/event.h>
#include <aio4c/log.h>
#include <aio4c/stats.h>
#include <aio4c/thread.h>
#include <aio4c/types.h>
#include <aio4c/writer.h>

#ifndef AIO4C_WIN32

#include <errno.h>

#endif /* AIO4C_WIN32 */

static aio4c_bool_t _WorkerInit(Worker* worker) {
    if ((worker->queue = NewQueue()) == NULL) {
        return false;
    }

    if ((worker->pool = NewBufferPool(worker->bufferSize)) == NULL) {
        return false;
    }

    if ((worker->writer = NewWriter(worker->pipe, worker->bufferSize)) == NULL) {
        return false;
    }

    Log(AIO4C_LOG_LEVEL_DEBUG, "initialized with tid 0x%08lx", worker->thread->id);

    return true;
}

static aio4c_bool_t _removeCallback(QueueItem* item, Connection* discriminant) {
    if (item->type == AIO4C_QUEUE_ITEM_TASK) {
        if (item->content.task.connection == discriminant) {
            ReleaseBuffer(&item->content.task.buffer);
            return true;
        }
    }

    return false;
}

static aio4c_bool_t _WorkerRun(Worker* worker) {
    QueueItem item;
    Connection* connection = NULL;

    while (Dequeue(worker->queue, &item, true)) {
        switch (item.type) {
            case AIO4C_QUEUE_ITEM_EXIT:
                return false;
            case AIO4C_QUEUE_ITEM_TASK:
                connection = item.content.task.connection;
                ProbeTimeStart(AIO4C_TIME_PROBE_DATA_PROCESS);
                connection->dataBuffer = item.content.task.buffer;
                ConnectionProcessData(connection);
                connection->dataBuffer = NULL;
                ProbeSize(AIO4C_PROBE_PROCESSED_DATA_SIZE,BufferGetPosition(item.content.task.buffer));
                ReleaseBuffer(&item.content.task.buffer);
                ProbeTimeEnd(AIO4C_TIME_PROBE_DATA_PROCESS);
                break;
            case AIO4C_QUEUE_ITEM_EVENT:
                connection = (Connection*)item.content.event.source;
                Log(AIO4C_LOG_LEVEL_DEBUG, "close received for connection %s", connection->string);
                RemoveAll(worker->queue, aio4c_remove_callback(_removeCallback), aio4c_remove_discriminant(connection));
                if (ConnectionNoMoreUsed(connection, AIO4C_CONNECTION_OWNER_WORKER)) {
                    Log(AIO4C_LOG_LEVEL_DEBUG, "freeing connection %s", connection->string);
                    FreeConnection(&connection);
                }
                break;
            default:
                break;
        }
    }

    return true;
}

static void _WorkerExit(Worker* worker) {
    if (worker->writer != NULL) {
        WriterEnd(worker->writer);
    }

    FreeQueue(&worker->queue);
    FreeBufferPool(&worker->pool);

    Log(AIO4C_LOG_LEVEL_DEBUG, "exited");
}

Worker* NewWorker(char* pipeName, aio4c_size_t bufferSize) {
    Worker* worker = NULL;
    ErrorCode code = AIO4C_ERROR_CODE_INITIALIZER;

    if ((worker = aio4c_malloc(sizeof(Worker))) == NULL) {
#ifndef AIO4C_WIN32
        code.error = errno;
#else /* AIO4C_WIN32 */
        code.source = AIO4C_ERRNO_SOURCE_SYS;
#endif /* AIO4C_WIN32 */
        code.size = sizeof(Worker);
        code.type = "Worker";
        Raise(AIO4C_LOG_LEVEL_ERROR, AIO4C_ALLOC_ERROR_TYPE, AIO4C_ALLOC_ERROR, &code);
        return NULL;
    }

    worker->queue      = NULL;
    worker->pool       = NULL;
    worker->writer     = NULL;
    worker->bufferSize = bufferSize;

    if (pipeName != NULL) {
        worker->pipe       = pipeName;
        worker->name       = aio4c_malloc(strlen(pipeName) + 1 + 7);
        if (worker->name != NULL) {
            snprintf(worker->name, strlen(pipeName) + 1 + 7, "%s-worker", pipeName);
        }
    } else {
        worker->pipe = NULL;
        worker->name = NULL;
    }

    worker->thread = NewThread(
            worker->name,
            aio4c_thread_init(_WorkerInit),
            aio4c_thread_run(_WorkerRun),
            aio4c_thread_exit(_WorkerExit),
            aio4c_thread_arg(worker));

    if (worker->thread == NULL) {
        if (worker->name != NULL) {
            aio4c_free(worker->name);
        }
        aio4c_free(worker);
        return NULL;
    }

    if (!ThreadStart(worker->thread)) {
        if (worker->name != NULL) {
            aio4c_free(worker->name);
        }
        aio4c_free(worker);
        return NULL;
    }

    return worker;
}

static void _WorkerCloseHandler(Event event, Connection* source, Worker* worker) {
    if (worker->queue == NULL || !EnqueueEventItem(worker->queue, event, source)) {
        if (ConnectionNoMoreUsed(source, AIO4C_CONNECTION_OWNER_WORKER)) {
            FreeConnection(&source);
        }
    }
}

static void _WorkerReadHandler(Event event, Connection* source, Worker* worker) {
    Buffer* bufferCopy = NULL;
    Event eventToProcess = AIO4C_OUTBOUND_DATA_EVENT;

    ProbeTimeStart(AIO4C_TIME_PROBE_DATA_PROCESS);

    if (event != AIO4C_INBOUND_DATA_EVENT || source->state == AIO4C_CONNECTION_STATE_CLOSED) {
        return;
    }

    bufferCopy = AllocateBuffer(worker->pool);

    if (event == AIO4C_INBOUND_DATA_EVENT) {
        BufferFlip(source->readBuffer);

        BufferCopy(bufferCopy, source->readBuffer);

        BufferReset(source->readBuffer);

        eventToProcess = AIO4C_READ_EVENT;
    }

    if (!EnqueueTaskItem(worker->queue, eventToProcess, source, bufferCopy)) {
        ReleaseBuffer(&bufferCopy);
        return;
    }

    ProbeTimeEnd(AIO4C_TIME_PROBE_DATA_PROCESS);
}

void WorkerManageConnection(Worker* worker, Connection* connection) {
    ConnectionAddSystemHandler(connection, AIO4C_INBOUND_DATA_EVENT, aio4c_connection_handler(_WorkerReadHandler), aio4c_connection_handler_arg(worker), false);
    ConnectionAddSystemHandler(connection, AIO4C_CLOSE_EVENT, aio4c_connection_handler(_WorkerCloseHandler), aio4c_connection_handler_arg(worker), true);
    ConnectionManagedBy(connection, AIO4C_CONNECTION_OWNER_WORKER);
    WriterManageConnection(worker->writer, connection);
}

void WorkerEnd(Worker* worker) {
    if (worker->thread != NULL) {
        if (worker->queue != NULL) {
            EnqueueExitItem(worker->queue);
        }

        ThreadJoin(worker->thread);
    }

    if (worker->name != NULL) {
        aio4c_free(worker->name);
    }

    aio4c_free(worker);
}

