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
    int i = 0;
    Buffer* bufferToProcess = NULL;
    Connection* connectionToProcess = NULL;

    TakeLock(worker->lock);

    while (worker->itemsCount <= 0 && WaitCondition(worker->condition, worker->lock));

    while (worker->itemsCount > 0) {
        bufferToProcess = worker->queue[0];
        connectionToProcess = worker->connections[0];

        ReleaseLock(worker->lock);

        ConnectionEventHandle(connectionToProcess, INBOUND_DATA_EVENT, bufferToProcess);

        TakeLock(worker->lock);

        for (i = 0; i < worker->itemsCount - 1; i++) {
            worker->queue[i] = worker->queue[i + 1];
            worker->connections[i] = worker->connections[i + 1];
        }

        BufferReset(bufferToProcess);

        worker->queue[i] = bufferToProcess;
        worker->connections[i] = NULL;
        worker->itemsCount--;

    }

    ReleaseLock(worker->lock);

    return true;
}

static void _WorkerExit(Worker* worker) {
    int i = 0;

    if (worker->writer != NULL) {
        FreeWriter(worker->thread, &worker->writer);
    }

    if (worker->itemsCount > 0) {
        for (i = 0; i < worker->itemsCount; i++) {
            if (worker->queue[i] != NULL) {
                FreeBuffer(&worker->queue[i]);
                worker->connections[i] = NULL;
            }
        }
        worker->itemsCount = 0;
    }

    Log(worker->thread, INFO, "exited");
}

Worker* NewWorker(Thread* parent, char* name, aio4c_size_t bufferSize) {
    Worker* worker = NULL;

    if ((worker = malloc(sizeof(Worker))) == NULL) {
        return NULL;
    }

    worker->queue = NULL;
    worker->connections = NULL;
    worker->queueSize = 0;
    worker->itemsCount = 0;
    worker->bufferSize = bufferSize;
    worker->condition = NewCondition();
    worker->lock = NewLock();
    worker->thread = NULL;
    worker->thread = NewThread(name, aio4c_thread_handler(_WorkerInit), aio4c_thread_run(_WorkerRun), aio4c_thread_handler(_WorkerExit), aio4c_thread_arg(worker));
    worker->writer = NULL;
    worker->writer = NewWriter(worker->thread, "writer", worker->bufferSize);

    return worker;
}

static void _WorkerReadHandler(Event event, Connection* source, Worker* worker) {
    Thread* current = ThreadSelf();
    Buffer* bufferCopy = NULL;

    TakeLock(worker->lock);

    if (worker->queueSize <= worker->itemsCount) {
        if ((worker->queue = realloc(worker->queue, (worker->queueSize + 1) * sizeof(Buffer*))) == NULL) {
            Log(current, WARN, "unable to alloc buffer queue: %s", strerror(errno));
            FreeBuffer(&bufferCopy);
            return;
        }

        if ((worker->connections = realloc(worker->connections, (worker->queueSize + 1) * sizeof(Connection*))) == NULL) {
            Log(current, WARN, "unable to allocate connection queue: %s", strerror(errno));
            FreeBuffer(&bufferCopy);
            return;
        }

        worker->queue[worker->queueSize] = NewBuffer(worker->bufferSize);
        worker->queueSize++;
    }

    BufferCopy(worker->queue[worker->itemsCount], source->readBuffer);
    worker->connections[worker->itemsCount] = source;
    worker->itemsCount++;

    NotifyCondition(worker->condition, worker->lock);

    ReleaseLock(worker->lock);
}

void WorkerManageConnection(Worker* worker, Connection* connection) {
    ConnectionAddSystemHandler(connection, READ_EVENT, aio4c_connection_handler(_WorkerReadHandler), aio4c_connection_handler_arg(worker), false);
    WriterManageConnection(worker->writer, connection);
}

void FreeWorker(Thread* parent, Worker** worker) {
    Worker* pWorker = NULL;
    int i = 0;

    if (worker != NULL && (pWorker = *worker) != NULL) {
        if (pWorker->thread != NULL) {
            ThreadCancel(pWorker->thread, true);
            FreeThread(&pWorker->thread);
        }

        if (pWorker->queue != NULL) {
            for (i = 0; i < pWorker->queueSize; i++) {
                if (pWorker->queue[i] != NULL) {
                    FreeBuffer(&pWorker->queue[i]);
                }
            }
            pWorker->queueSize = 0;
            pWorker->itemsCount = 0;
            free(pWorker->queue);
        }

        if (pWorker->connections != NULL) {
            free(pWorker->connections);
        }

        if (pWorker->condition != NULL) {
            FreeCondition(&pWorker->condition);
        }

        if (pWorker->lock != NULL) {
            FreeLock(&pWorker->lock);
        }

        free(pWorker);
        *worker = NULL;
    }
}

