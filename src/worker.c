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
#include <aio4c/worker.h>

#include <aio4c/alloc.h>
#include <aio4c/buffer.h>
#include <aio4c/connection.h>
#include <aio4c/event.h>
#include <aio4c/log.h>
#include <aio4c/stats.h>
#include <aio4c/thread.h>
#include <aio4c/types.h>
#include <aio4c/writer.h>

#include <errno.h>
#include <stdlib.h>
#include <string.h>

static void _WorkerInit(Worker* worker) {
    Log(AIO4C_LOG_LEVEL_INFO, "initialized with tid 0x%08lx", worker->thread->id);
}

static aio4c_bool_t _removeCallback(QueueItem* item, Connection* discriminant) {
    Task* task = NULL;
    if (item->type == AIO4C_QUEUE_ITEM_DATA) {
        task = (Task*)item->content.data;
        if (task->connection == discriminant) {
            ReleaseBuffer(&task->buffer);
            aio4c_free(task);
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
                ProbeTimeStart(AIO4C_TIME_PROBE_DATA_PROCESS);
                item.content.task.connection->dataBuffer = item.content.task.buffer;
                ConnectionEventHandle(item.content.task.connection, item.content.task.event, item.content.task.connection);
                item.content.task.connection->dataBuffer = NULL;
                ProbeSize(AIO4C_PROBE_PROCESSED_DATA_SIZE,item.content.task.buffer->position);
                ReleaseBuffer(&item.content.task.buffer);
                ProbeTimeEnd(AIO4C_TIME_PROBE_DATA_PROCESS);
                break;
            case AIO4C_QUEUE_ITEM_EVENT:
                connection = (Connection*)item.content.event.source;
                Log(AIO4C_LOG_LEVEL_DEBUG, "close received for connection %s", connection->string);
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
    FreeQueue(&worker->queue);
    FreeBufferPool(&worker->pool);

    WriterEnd(worker->writer);

    Log(AIO4C_LOG_LEVEL_INFO, "exited");

    aio4c_free(worker);
}

Worker* NewWorker(aio4c_size_t bufferSize) {
    Worker* worker = NULL;

    if ((worker = aio4c_malloc(sizeof(Worker))) == NULL) {
        return NULL;
    }

    worker->queue = NewQueue();
    worker->bufferSize = bufferSize;
    worker->writer = NULL;
    worker->writer = NewWriter(worker->bufferSize);
    worker->thread = NULL;
    NewThread("worker",
            aio4c_thread_handler(_WorkerInit),
            aio4c_thread_run(_WorkerRun),
            aio4c_thread_handler(_WorkerExit),
            aio4c_thread_arg(worker),
            &worker->thread);
    worker->pool = NewBufferPool(4, bufferSize);

    return worker;
}

static void _WorkerCloseHandler(Event event, Connection* source, Worker* worker) {
    RemoveAll(worker->queue, aio4c_remove_callback(_removeCallback), aio4c_remove_discriminant(source));

    EnqueueEventItem(worker->queue, event, source);
}

static void _WorkerReadHandler(Event event, Connection* source, Worker* worker) {
    Buffer* bufferCopy = NULL;
    Event eventToProcess = AIO4C_OUTBOUND_DATA_EVENT;

    ProbeTimeStart(AIO4C_TIME_PROBE_DATA_PROCESS);

    if (event != AIO4C_READ_EVENT || source->state == AIO4C_CONNECTION_STATE_CLOSED) {
        return;
    }

    bufferCopy = AllocateBuffer(worker->pool);

    if (event == AIO4C_READ_EVENT) {
        BufferFlip(source->readBuffer);

        BufferCopy(bufferCopy, source->readBuffer);

        BufferReset(source->readBuffer);

        eventToProcess = AIO4C_INBOUND_DATA_EVENT;
    }

    if (!EnqueueTaskItem(worker->queue, eventToProcess, source, bufferCopy)) {
        ReleaseBuffer(&bufferCopy);
        return;
    }

    ProbeTimeEnd(AIO4C_TIME_PROBE_DATA_PROCESS);
}

void WorkerManageConnection(Worker* worker, Connection* connection) {
    ConnectionAddSystemHandler(connection, AIO4C_READ_EVENT, aio4c_connection_handler(_WorkerReadHandler), aio4c_connection_handler_arg(worker), false);
    ConnectionAddSystemHandler(connection, AIO4C_CLOSE_EVENT, aio4c_connection_handler(_WorkerCloseHandler), aio4c_connection_handler_arg(worker), true);
    WriterManageConnection(worker->writer, connection);
}

void WorkerEnd(Worker* worker) {
    Thread* th = worker->thread;

    if (!EnqueueExitItem(worker->queue)) {
        return;
    }

    ThreadJoin(th);
}

