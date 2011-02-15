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

static void _ReaderWakeUp(Thread* from, Reader* reader) {
    unsigned char dummy = 1;

    if (write(reader->selector[AIO4C_PIPE_WRITE], &dummy, sizeof(unsigned char)) <= 0) {
        Log(from, WARN, "wake up reader %s: %s", reader->thread->name, strerror(errno));
    }

    Log(from, DEBUG, "notified reader %s", reader->thread->name);
}

static void _ReaderWokenUp(Reader* reader) {
    unsigned char dummy = 0;
    aio4c_size_t nbNotifies = 0;

    errno = 0;

    while (read(reader->selector[AIO4C_PIPE_READ], &dummy, sizeof(unsigned char)) > 0) {
        nbNotifies++;
    }

    Log(reader->thread, DEBUG, "%d notifications received", nbNotifies);
}

static void _ReaderInit(Reader* reader) {
    Log(reader->thread, DEBUG, "reader initialized");
}

static aio4c_bool_t _ReaderRun(Reader* reader) {
    int numConnectionsReady = 0, i = 0, soError = 0;
    socklen_t soLen = sizeof(int);
    TakeLock(reader->thread, reader->lock);

    while (reader->numConnections <= 0) {
        WaitCondition(reader->thread, reader->condition, reader->lock);
    }

    ReleaseLock(reader->lock);
    numConnectionsReady = poll(reader->polling, reader->numConnections, -1);
    TakeLock(reader->thread, reader->lock);
    if (numConnectionsReady == 0 || (numConnectionsReady == -1 && errno == EINTR)) {
        Log(reader->thread, WARN, "polling interrupted, retrying");
        ReleaseLock(reader->lock);
        return true;
    }

    if (numConnectionsReady == -1) {
        Log(reader->thread, ERROR, "polling: %s", strerror(errno));
        ReleaseLock(reader->lock);
        return false;
    }

    if (reader->polling[0].revents == POLLIN) {
        _ReaderWokenUp(reader);
        reader->polling[0].revents = 0;
        reader->polling[0].events = POLLIN;
    }

    for (i = 1; i < reader->numConnections; i++) {
        if (reader->polling[i].revents == POLLIN) {
            ConnectionRead(reader->connections[i]);
        } else if (reader->polling[i].revents & POLLERR) {
            getsockopt(reader->connections[i]->socket, SOL_SOCKET, SO_ERROR, &soError, &soLen);
            Log(reader->thread, WARN, "error on connection read: %s", strerror(soError));
        } else if (reader->polling[i].revents & POLLHUP) {
            Log(reader->thread, WARN, "connection disconnected");
        }

        reader->polling[i].revents = 0;
        reader->polling[i].events = POLLIN;
    }

    ReleaseLock(reader->lock);
    return true;
}

static void _ReaderExit(Reader* reader) {
    int i = 0;
    Thread* readerThread = reader->thread;

    TakeLock(reader->thread, reader->lock);

    for (i = 0; i < reader->maxConnections; i ++) {
        if (reader->connections[i] != NULL) {
            if (reader->connections[i]->state != CLOSED) {
                ConnectionClose(reader->connections[i]);
                FreeConnection(&reader->connections[i]);
            }
        }
    }

    free(reader->connections);
    free(reader->polling);

    close(reader->selector[AIO4C_PIPE_READ]);
    close(reader->selector[AIO4C_PIPE_WRITE]);

    FreeWorker(reader->thread, &reader->worker);

    FreeLock(&reader->lock);

    FreeCondition(&reader->condition);

    free(reader);

    Log(readerThread, DEBUG, "reader exited");
}

Reader* NewReader(Thread* parent, char* name, aio4c_size_t bufferSize) {
    Reader* reader = NULL;

    if ((reader = malloc(sizeof(Reader))) == NULL) {
        Log(parent, ERROR, "reader allocation: %s", strerror(errno));
        return NULL;
    }

    if ((reader->connections = malloc(sizeof(Connection*))) == NULL) {
        Log(parent, ERROR, "reader allocation: %s", strerror(errno));
        free(reader);
        return NULL;
    }

    if ((reader->polling = malloc(sizeof(aio4c_poll_t))) == NULL) {
        Log(parent, ERROR, "reader allocation: %s", strerror(errno));
        free(reader->connections);
        free(reader);
        return NULL;
    }

    reader->numConnections = 1;
    reader->maxConnections = 1;
    reader->lock = NewLock();
    reader->condition = NewCondition();

    if (pipe(reader->selector) != 0) {
        Log(parent, ERROR, "reader selector creation: %s", strerror(errno));
        FreeLock(&reader->lock);
        FreeCondition(&reader->condition);
        free(reader->polling);
        free(reader->connections);
        return NULL;
    }

    if (fcntl(reader->selector[AIO4C_PIPE_READ], F_SETFL, O_NONBLOCK) != 0) {
        Log(parent, WARN, "reader read selector non-block: %s", strerror(errno));
    }

    if (fcntl(reader->selector[AIO4C_PIPE_WRITE], F_SETFL, O_NONBLOCK) != 0) {
        Log(parent, WARN, "reader write selector non-block: %s", strerror(errno));
    }

    reader->connections[0] = NULL;
    reader->polling[0].fd = reader->selector[AIO4C_PIPE_READ];
    reader->polling[0].events = POLLIN;
    reader->polling[0].revents = 0;
    reader->thread = NewThread(parent, name, aio4c_thread_handler(_ReaderInit), aio4c_thread_run(_ReaderRun), aio4c_thread_handler(_ReaderExit), aio4c_thread_arg(reader));
    reader->worker = NewWorker(reader->thread, "worker", bufferSize);

    return reader;
}

static void _ReaderCloseHandler(Event event, Connection* connection, Reader* reader) {
    int i = 0;
    Thread* currentThread = ThreadSelf(NULL);

    TakeLock(currentThread, reader->lock);

    if (currentThread != reader->thread) {
        _ReaderWakeUp(currentThread, reader);
    }

    for (i = 1; i < reader->numConnections; i++) {
        if (reader->connections[i] == connection) {
            break;
        }
    }

    if (i < reader->numConnections) {
        for (; i < reader->numConnections - 1; i++) {
            reader->connections[i] = reader->connections[i + 1];
            memcpy(&reader->polling[i], &reader->polling[i + 1], sizeof(aio4c_poll_t));
        }

        reader->connections[i] = NULL;
        memset(&reader->polling[i], 0, sizeof(aio4c_poll_t));

        reader->numConnections--;
    }

    ReleaseLock(reader->lock);
}

void ReaderManageConnection(Thread* from, Reader* reader, Connection* connection) {
    TakeLock(from, reader->lock);

    _ReaderWakeUp(from, reader);

    if (reader->numConnections >= reader->maxConnections) {
        if ((reader->connections = realloc(reader->connections, (reader->maxConnections + 1) * sizeof(Connection*))) == NULL) {
            ReleaseLock(reader->lock);
            return;
        }

        if ((reader->polling = realloc(reader->polling, (reader->maxConnections + 1) * sizeof(aio4c_poll_t))) == NULL) {
            ReleaseLock(reader->lock);
            return;
        }

        reader->maxConnections ++;
    }

    reader->connections[reader->numConnections] = connection;
    reader->polling[reader->numConnections].events = POLLIN;
    reader->polling[reader->numConnections].revents = 0;
    reader->polling[reader->numConnections].fd = connection->socket;
    reader->numConnections ++;

    ConnectionAddHandler(connection, CLOSE_EVENT, aio4c_connection_handler(_ReaderCloseHandler), aio4c_connection_handler_arg(reader), true);

    WorkerManageConnection(reader->worker, connection);

    NotifyCondition(from, reader->condition, reader->lock);

    ReleaseLock(reader->lock);
}
