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
    Log(reader->thread, DEBUG, "reader initialized");
}

static aio4c_bool_t _ReaderRun(Reader* reader) {
    int numConnectionsReady = 0, i = 0, soError = 0;
    unsigned char dummy = 0;
    socklen_t soLen = sizeof(int);
    aio4c_bool_t closeConnection = false;
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
        if (read(reader->pipe[0], &dummy, sizeof(unsigned char)) < 0) {
            perror("read pipe");
        }
        printf("notified from pipe !\n");
        dummy = 0;
        reader->polling[0].revents = 0;
        reader->polling[0].events = POLLIN;
    }

    for (i = 1; i < reader->numConnections; i++) {
        closeConnection = false;
        if (reader->polling[i].revents == POLLIN) {
            ConnectionRead(reader->connections[i]);
        } else if (reader->polling[i].revents & POLLERR) {
            getsockopt(reader->connections[i]->socket, SOL_SOCKET, SO_ERROR, &soError, &soLen);
            Log(reader->thread, WARN, "error on connection read: %s", strerror(soError));
            closeConnection = true;
        } else if (reader->polling[i].revents & POLLHUP) {
            Log(reader->thread, WARN, "connection disconnected");
            closeConnection = true;
        }

        reader->polling[i].revents = 0;
        reader->polling[i].events = POLLIN;

        if (closeConnection) {
            ConnectionClose(reader->connections[i]);
        }
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

    FreeLock(&reader->lock);

    FreeLock(&reader->lock);
    FreeCondition(&reader->condition);

    free(reader);

    Log(readerThread, DEBUG, "reader exited");
}

Reader* NewReader(Thread* parent) {
    Reader* reader = NULL;

    if ((reader = malloc(sizeof(Reader))) == NULL) {
        return NULL;
    }

    reader->connections = malloc(sizeof(Connection*));
    if (reader->connections == NULL) {
        perror("malloc connections");
    }
    reader->polling = malloc(sizeof(aio4c_poll_t));
    if (reader->polling == NULL) {
        perror("malloc polling");
    }
    reader->numConnections = 1;
    reader->maxConnections = 1;
    reader->lock = NewLock();
    reader->condition = NewCondition();
    if (pipe(reader->pipe) != 0) {
        perror("pipe");
    }
    if (fcntl(reader->pipe[0], F_SETFL, O_NONBLOCK) != 0) {
        perror("pipe");
    }
    if (fcntl(reader->pipe[1], F_SETFL, O_NONBLOCK) != 0) {
        perror("pipe");
    }
    reader->connections[0] = NULL;
    reader->polling[0].fd = reader->pipe[0];
    reader->polling[0].events = POLLIN;
    reader->polling[0].revents = 0;
    reader->thread = NewThread(parent, "reader", aio4c_thread_handler(_ReaderInit), aio4c_thread_run(_ReaderRun), aio4c_thread_handler(_ReaderExit), aio4c_thread_arg(reader));

    return reader;
}

void _ReaderCloseHandler(Event event, Connection* connection, Reader* reader) {
    unsigned char dummy = 1;
    int i = 0;

    TakeLock(reader->thread, reader->lock);
    if (write(reader->pipe[1], &dummy, sizeof(unsigned char)) < 0) {
        perror("write pipe");
    }

    for (i = 1; i < reader->numConnections; i++) {
        if (reader->connections[i] == connection) {
            break;
        }
    }

    if (i < reader->numConnections) {
        printf("removing connection %d\n", i);
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
    unsigned char dummy = 1;
    TakeLock(from, reader->lock);
    if (write(reader->pipe[1], &dummy, sizeof(unsigned char)) < 0) {
        perror("write pipe");
    }

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

    NotifyCondition(from, reader->condition, reader->lock);

    ReleaseLock(reader->lock);
}
