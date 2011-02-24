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
    Log(worker->thread, INFO, "initialized");
}

static aio4c_bool_t _removeCallback(QueueItem* item, Connection* discriminant) {
    Task* task = NULL;
    if (item->type == DATA) {
        task = (Task*)item->content.data;
        if (task->connection == discriminant) {
            FreeBuffer(&task->buffer);
            aio4c_free(task);
            return true;
        }
    }

    return false;
}

static aio4c_bool_t _WorkerRun(Worker* worker) {
    QueueItem item;
    Task* task = NULL;
    Connection* connection = NULL;

    while (Dequeue(worker->queue, &item, true)) {
        switch (item.type) {
            case EXIT:
                return false;
            case DATA:
                task = (Task*)item.content.data;
                ProbeTimeStart(TIME_PROBE_DATA_PROCESS);
                task->connection->dataBuffer = task->buffer;
                ConnectionEventHandle(task->connection, INBOUND_DATA_EVENT, task->connection);
                task->connection->dataBuffer = NULL;
                ProbeSize(PROBE_PROCESSED_DATA_SIZE,task->buffer->position);
                ProbeTimeEnd(TIME_PROBE_DATA_PROCESS);
                FreeBuffer(&task->buffer);
                aio4c_free(task);
                break;
            case EVENT:
                connection = (Connection*)item.content.event.source;
                Log(worker->thread, DEBUG, "close received for connection %s", connection->string);
                if (ConnectionNoMoreUsed(connection, WORKER)) {
                    Log(worker->thread, DEBUG, "freeing connection %s", connection->string);
                    FreeConnection(&connection);
                }
                break;
        }
    }

    return true;
}

static void _WorkerExit(Worker* worker) {
    FreeQueue(&worker->queue);

    WriterEnd(worker->writer);

    Log(worker->thread, INFO, "exited");

    aio4c_free(worker);
}

Worker* NewWorker(char* name, aio4c_size_t bufferSize) {
    Worker* worker = NULL;

    if ((worker = aio4c_malloc(sizeof(Worker))) == NULL) {
        return NULL;
    }

    worker->queue = NewQueue();
    worker->bufferSize = bufferSize;
    worker->writer = NULL;
    worker->writer = NewWriter(worker->thread, "writer", worker->bufferSize);
    worker->thread = NULL;
    worker->thread = NewThread(name, aio4c_thread_handler(_WorkerInit), aio4c_thread_run(_WorkerRun), aio4c_thread_handler(_WorkerExit), aio4c_thread_arg(worker));

    return worker;
}

static void _WorkerCloseHandler(Event event, Connection* source, Worker* worker) {
    RemoveAll(worker->queue, aio4c_remove_callback(_removeCallback), aio4c_remove_discriminant(source));

    EnqueueEventItem(worker->queue, event, source);
}

static void _WorkerReadHandler(Event event, Connection* source, Worker* worker) {
    Buffer* bufferCopy = NULL;
    Task* task = NULL;

    if (event != READ_EVENT || source->state == CLOSED) {
        return;
    }

    if ((task = aio4c_malloc(sizeof(Task))) == NULL) {
        return;
    }

    bufferCopy = NewBuffer(worker->bufferSize);

    BufferFlip(source->readBuffer);

    BufferCopy(bufferCopy, source->readBuffer);

    BufferReset(source->readBuffer);

    task->connection = source;
    task->buffer = bufferCopy;

    if (!EnqueueDataItem(worker->queue, task)) {
        FreeBuffer(&bufferCopy);
        aio4c_free(task);
        return;
    }
}

void WorkerManageConnection(Worker* worker, Connection* connection) {
    ConnectionAddSystemHandler(connection, READ_EVENT, aio4c_connection_handler(_WorkerReadHandler), aio4c_connection_handler_arg(worker), false);
    ConnectionAddSystemHandler(connection, CLOSE_EVENT, aio4c_connection_handler(_WorkerCloseHandler), aio4c_connection_handler_arg(worker), true);
    WriterManageConnection(worker->writer, connection);
}

void WorkerEnd(Worker* worker) {
    Thread* th = worker->thread;

    if (!EnqueueExitItem(worker->queue)) {
        return;
    }

    ThreadJoin(th);
    FreeThread(&th);
}

