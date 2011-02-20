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

static aio4c_bool_t _WorkerRun(Worker* worker) {
    QueueItem* item = NULL;
    Task* task = NULL;

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
                ConnectionEventHandle(item->content.event.source, item->content.event.type, item->content.event.source);
                break;
        }
        FreeItem(&item);
    }

    return true;
}

static void _WorkerExit(Worker* worker) {
    Thread* writer = worker->writer->thread;

    FreeQueue(&worker->queue);

    WriterEnd(worker->writer);
    ThreadJoin(writer);
    FreeThread(&writer);

    Log(worker->thread, INFO, "exited");

    free(worker);
}

Worker* NewWorker(Thread* parent, char* name, aio4c_size_t bufferSize) {
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

static void _WorkerReadHandler(Event event, Connection* source, Worker* worker) {
    Buffer* bufferCopy = NULL;
    Task* task = NULL;
    QueueItem* item = NULL;

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

    Enqueue(worker->queue, item);
}

void WorkerManageConnection(Worker* worker, Connection* connection) {
    ConnectionAddSystemHandler(connection, READ_EVENT, aio4c_connection_handler(_WorkerReadHandler), aio4c_connection_handler_arg(worker), false);
    WriterManageConnection(worker->writer, connection);
}

void WorkerEnd(Worker* worker) {
    QueueItem* item = NewExitItem();

    Enqueue(worker->queue, item);
}

