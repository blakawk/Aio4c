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

#include <aio4c/buffer.h>
#include <aio4c/connection.h>
#include <aio4c/event.h>
#include <aio4c/log.h>
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
            return true;
        }
    }

    return false;
}

static aio4c_bool_t _WorkerRun(Worker* worker) {
    QueueItem* item = NULL;
    Task* task = NULL;
    Connection* connection = NULL;

    while (Dequeue(worker->queue, &item, true)) {
        switch (item->type) {
            case EXIT:
                Log(worker->thread, DEBUG, "read EXIT message");
                FreeItem(&item);
                return false;
            case DATA:
                task = (Task*)item->content.data;
                ConnectionEventHandle(task->connection, INBOUND_DATA_EVENT, task->buffer);
                FreeBuffer(&task->buffer);
                free(task);
                break;
            case EVENT:
                connection = (Connection*)item->content.event.source;
                Log(worker->thread, DEBUG, "close for connection %s received", connection->string);
                RemoveAll(worker->queue, aio4c_remove_callback(_removeCallback), aio4c_remove_discriminant(connection));
                if (ConnectionNoMoreUsed(connection, WORKER)) {
                    Log(worker->thread, DEBUG, "freeing connection %s", connection->string);
                    FreeConnection(&connection);
                }
                break;
        }
        FreeItem(&item);
    }

    return true;
}

static void _WorkerExit(Worker* worker) {
    FreeQueue(&worker->queue);

    WriterEnd(worker->writer);

    Log(worker->thread, INFO, "exited");

    free(worker);
}

Worker* NewWorker(char* name, aio4c_size_t bufferSize) {
    Worker* worker = NULL;

    if ((worker = malloc(sizeof(Worker))) == NULL) {
        return NULL;
    }

    worker->queue = NewQueue();
    worker->bufferSize = bufferSize;
    worker->thread = NULL;
    worker->thread = NewThread(name, aio4c_thread_handler(_WorkerInit), aio4c_thread_run(_WorkerRun), aio4c_thread_handler(_WorkerExit), aio4c_thread_arg(worker));
    worker->writer = NULL;
    worker->writer = NewWriter(worker->thread, "writer", worker->bufferSize);

    return worker;
}

static void _WorkerCloseHandler(Event event, Connection* source, Worker* worker) {
    QueueItem* item = NULL;

    if (!Enqueue(worker->queue, item = NewEventItem(event, source))) {
        FreeItem(&item);
        return;
    }
}

static void _WorkerReadHandler(Event event, Connection* source, Worker* worker) {
    Buffer* bufferCopy = NULL;
    Task* task = NULL;
    QueueItem* item = NULL;

    if (event != READ_EVENT) {
        return;
    }

    if ((task = malloc(sizeof(Task))) == NULL) {
        return;
    }

    bufferCopy = NewBuffer(worker->bufferSize);

    BufferFlip(source->readBuffer);

    BufferCopy(bufferCopy, source->readBuffer);

    BufferReset(source->readBuffer);

    task->connection = source;
    task->buffer = bufferCopy;

    item = NewDataItem((void*)task);

    if (!Enqueue(worker->queue, item)) {
        FreeItem(&item);
        FreeBuffer(&bufferCopy);
        free(task);
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

    QueueItem* item = NewExitItem();

    if (!Enqueue(worker->queue, item)) {
        FreeItem(&item);
        return;
    }

    ThreadJoin(th);
    FreeThread(&th);
}

