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

#include <aio4c/connection.h>
#include <aio4c/log.h>
#include <aio4c/thread.h>
#include <aio4c/types.h>

#include <errno.h>
#include <fcntl.h>
#include <poll.h>
#include <signal.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

static void _ReaderInit(Reader* reader) {
    Log(reader->thread, INFO, "initialized");
}

static aio4c_bool_t _ReaderRun(Reader* reader) {
    QueueItem* item = NULL;
    Connection* connection = NULL;
    SelectionKey* key = NULL;
    int numConnectionsReady = 0;

    while(Dequeue(reader->queue, &item, false)) {
        switch(item->type) {
            case EXIT:
                Log(reader->thread, DEBUG, "read EXIT message");
                FreeItem(&item);
                return false;
            case DATA:
                connection = (Connection*)item->content.data;
                Log(reader->thread, DEBUG, "registering connection %s", connection->string);
                Register(reader->selector, AIO4C_OP_READ, connection->socket, (void*)connection);
                break;
            case EVENT:
                connection = (Connection*)item->content.event.source;
                Log(reader->thread, DEBUG, "close for connection %s received", connection->string);
                break;
        }

        FreeItem(&item);
    }

    numConnectionsReady = Select(reader->selector);

    if (numConnectionsReady > 0) {
        while (SelectionKeyReady(reader->selector, &key)) {
            Log(reader->thread, DEBUG, "connection %s ready", ((Connection*)key->attachment)->string);
            if (key->result == key->operation) {
                ConnectionRead((Connection*)key->attachment);
            } else {
                ConnectionClose((Connection*)key->attachment);
                Unregister(reader->selector, key);
            }
        }
    }

    return true;
}

static void _ReaderExit(Reader* reader) {
    Thread* worker = reader->worker->thread;

    WorkerEnd(reader->worker);
    ThreadJoin(worker);
    FreeThread(&worker);

    FreeQueue(&reader->queue);
    FreeSelector(&reader->selector);

    Log(reader->thread, INFO, "exited");

    free(reader);
}

Reader* NewReader(Thread* parent, char* name, aio4c_size_t bufferSize) {
    Reader* reader = NULL;

    if ((reader = malloc(sizeof(Reader))) == NULL) {
        Log(parent, ERROR, "reader allocation: %s", strerror(errno));
        return NULL;
    }

    reader->selector = NewSelector();
    reader->queue = NewQueue();
    reader->thread = NULL;
    reader->thread = NewThread(name, aio4c_thread_handler(_ReaderInit), aio4c_thread_run(_ReaderRun), aio4c_thread_handler(_ReaderExit), aio4c_thread_arg(reader));
    reader->worker = NULL;
    reader->worker = NewWorker(reader->thread, "worker", bufferSize);

    return reader;
}

static void _ReaderEventHandler(Event event, Connection* connection, Reader* reader) {
    Enqueue(reader->queue, NewEventItem(event, connection));

    SelectorWakeUp(reader->selector);
}

void ReaderManageConnection(Thread* from, Reader* reader, Connection* connection) {
    QueueItem* item = NewDataItem((void*)connection);

    Enqueue(reader->queue, item);

    ConnectionAddSystemHandler(connection, CLOSE_EVENT, aio4c_connection_handler(_ReaderEventHandler), aio4c_connection_handler_arg(reader), true);

    WorkerManageConnection(reader->worker, connection);

    SelectorWakeUp(reader->selector);
}

void ReaderEnd(Reader* reader) {
    Enqueue(reader->queue, NewExitItem());

    SelectorWakeUp(reader->selector);
}

