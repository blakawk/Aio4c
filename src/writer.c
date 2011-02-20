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
#include <aio4c/writer.h>

#include <aio4c/buffer.h>
#include <aio4c/connection.h>
#include <aio4c/event.h>
#include <aio4c/log.h>
#include <aio4c/thread.h>

#include <errno.h>
#include <fcntl.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>

static void _WriterWakeUp(Thread* from, Writer* writer) {
    unsigned char dummy = 1;

    if (writer->thread == NULL) {
        return;
    }

    if (write(writer->selector[AIO4C_PIPE_WRITE], &dummy, sizeof(unsigned char)) <= 0) {
        Log(from, WARN, "wake up writer %s: %s", writer->thread->name, strerror(errno));
    }

    Log(from, DEBUG, "notified writer %s", writer->thread->name);
}

static void _WriterWokenUp(Writer* writer) {
    unsigned char dummy = 0;
    aio4c_size_t nbNotifies = 0;

    errno = 0;

    while (read(writer->selector[AIO4C_PIPE_READ], &dummy, sizeof(unsigned char)) > 0) {
        nbNotifies++;
    }

    Log(writer->thread, DEBUG, "%d notifications received", nbNotifies);
}

static void _WriterInit(Writer* writer) {
    Log(writer->thread, INFO, "initialized");
}

static void _WriterConnectionQueue(Writer* writer, Connection* connection, Buffer* buffer) {
    if (writer->numConnections >= writer->maxConnections) {
        if ((writer->connections = realloc(writer->connections, (writer->maxConnections + 1) * sizeof(Connection*))) == NULL) {
            Log(writer->thread, ERROR, "cannot allocate connection: %s", strerror(errno));
            return;
        }

        if ((writer->polling = realloc(writer->polling, (writer->maxConnections + 1) * sizeof(aio4c_poll_t))) == NULL) {
            Log(writer->thread, ERROR, "cannot allocate polling: %s", strerror(errno));
            return;
        }

        if ((writer->buffers = realloc(writer->buffers, (writer->maxConnections + 1) * sizeof(Buffer*))) == NULL) {
            Log(writer->thread, ERROR, "cannot allocate buffers: %s", strerror(errno));
            return;
        }

        writer->buffers[writer->maxConnections] = NewBuffer(writer->bufferSize);
        writer->maxConnections++;
    }

    writer->connections[writer->numConnections] = connection;
    writer->polling[writer->numConnections].fd = connection->socket;
    writer->polling[writer->numConnections].events = POLLOUT;
    writer->polling[writer->numConnections].revents = 0;
    if (buffer == NULL) {
        BufferCopy(writer->buffers[writer->numConnections], connection->writeBuffer);
    } else {
        BufferCopy(writer->buffers[writer->numConnections], buffer);
    }
    writer->numConnections++;
}

static aio4c_bool_t _WriterRun(Writer* writer) {
    int numConnectionsReady = 0, numConnections = 0, i = 0, soError = 0;
    socklen_t soLen = sizeof(int);

    TakeLock(writer->lock);

    while (writer->numConnections <= 1 && WaitCondition(writer->condition, writer->lock));

    ReleaseLock(writer->lock);
    numConnectionsReady = poll(writer->polling, writer->numConnections, -1);
    TakeLock(writer->lock);

    if (numConnectionsReady == 0 || (numConnectionsReady == -1 && errno == EINTR)) {
        Log(writer->thread, WARN, "polling interrupted, retrying");
        ReleaseLock(writer->lock);
        return true;
    }

    if (numConnectionsReady == -1) {
        Log(writer->thread, ERROR, "polling: %s", strerror(errno));
        ReleaseLock(writer->lock);
        return false;
    }

    if (writer->polling[0].revents == POLLIN) {
        _WriterWokenUp(writer);
        writer->polling[0].revents = 0;
        writer->polling[0].events = POLLIN;
    }

    numConnections = writer->numConnections;
    for (i= 1; i < numConnections; i++) {
        if (writer->polling[i].revents == POLLOUT) {
            if (writer->connections[i]->state == CONNECTING) {
                ConnectionFinishConnect(writer->connections[i]);
            } else {
                ConnectionWrite(writer->connections[i], writer->buffers[i]);

                if (writer->buffers[i]->position < writer->buffers[i]->limit) {
                    _WriterConnectionQueue(writer, writer->connections[i], writer->buffers[i]);
                }
            }
        } else if (writer->polling[i].revents & POLLERR) {
            if (getsockopt(writer->connections[i]->socket, SOL_SOCKET, SO_ERROR, &soError, &soLen) != 0) {
                Log(writer->thread, WARN, "error on connection %s write: %s", writer->connections[i]->string, strerror(errno));
            } else {
                Log(writer->thread, WARN, "error on connection %s write: %s", writer->connections[i]->string, strerror(soError));
            }
        } else if (writer->polling[i].revents & POLLHUP) {
            Log(writer->thread, WARN, "connection %s disconnected", writer->connections[i]->string);
        }

        writer->numConnections--;
    }

    if (writer->numConnections > 1) {
        memcpy(&writer->connections[1], &writer->connections[writer->numConnections], (writer->numConnections - 1) * sizeof(Connection*));
        memcpy(&writer->polling[1], &writer->polling[writer->numConnections], (writer->numConnections - 1) * sizeof(aio4c_poll_t));
        memcpy(&writer->buffers[1], &writer->buffers[writer->numConnections], (writer->numConnections - 1) * sizeof(Buffer*));
    }

    ReleaseLock(writer->lock);

    return true;
}

static void _WriterExit(Writer* writer) {
    int i = 0;

    for (i = 0; i < writer->numConnections; i++) {
        if (writer->connections[i] != NULL) {
            if (writer->connections[i]->state != CLOSED) {
                ConnectionClose(writer->connections[i]);
                FreeConnection(&writer->connections[i]);
            }
        }

        if (writer->buffers[i] != NULL) {
            FreeBuffer(&writer->buffers[i]);
        }
    }

    writer->numConnections = 0;

    Log(writer->thread, INFO, "exited");
}

Writer* NewWriter(Thread* parent, char* name, aio4c_size_t bufferSize) {
    Writer* writer = NULL;

    if ((writer = malloc(sizeof(Writer))) == NULL) {
        Log(parent, ERROR, "cannot allocate writer: %s", strerror(errno));
        return NULL;
    }

    if (pipe(writer->selector) != 0) {
        Log(parent, ERROR, "cannot create writer selector: %s", strerror(errno));
        free(writer);
        return NULL;
    }

    if (fcntl(writer->selector[AIO4C_PIPE_READ], F_SETFL, O_NONBLOCK) != 0) {
        Log(parent, WARN, "writer read selector non-block: %s", strerror(errno));
    }

    if (fcntl(writer->selector[AIO4C_PIPE_WRITE], F_SETFL, O_NONBLOCK) != 0) {
        Log(parent, WARN, "writer write selector non-block: %s", strerror(errno));
    }

    if ((writer->polling = malloc(sizeof(aio4c_poll_t))) == NULL) {
        Log(parent, ERROR, "cannot allocate writer polling: %s", strerror(errno));
        close(writer->selector[AIO4C_PIPE_READ]);
        close(writer->selector[AIO4C_PIPE_WRITE]);
        free(writer);
        return NULL;
    }

    if ((writer->buffers = malloc(sizeof(Buffer*))) == NULL) {
        Log(parent, ERROR, "cannot allocate writer buffers: %s", strerror(errno));
        close(writer->selector[AIO4C_PIPE_READ]);
        close(writer->selector[AIO4C_PIPE_WRITE]);
        free(writer->polling);
        free(writer);
        return NULL;
    }

    if ((writer->connections = malloc(sizeof(Connection*))) == NULL) {
        Log(parent, ERROR, "cannot allocate writer connections: %s", strerror(errno));
        free(writer->polling);
        free(writer->buffers);
        close(writer->selector[AIO4C_PIPE_READ]);
        close(writer->selector[AIO4C_PIPE_WRITE]);
        free(writer);
        return NULL;
    }

    writer->lock = NewLock();
    writer->condition = NewCondition();

    writer->polling[0].fd = writer->selector[AIO4C_PIPE_READ];
    writer->polling[0].events = POLLIN;
    writer->polling[0].revents = 0;
    writer->connections[0] = NULL;
    writer->buffers[0] = NULL;
    writer->numConnections = 1;
    writer->maxConnections = 1;
    writer->bufferSize = bufferSize;

    writer->thread = NULL;
    writer->thread = NewThread(name, aio4c_thread_handler(_WriterInit), aio4c_thread_run(_WriterRun), aio4c_thread_handler(_WriterExit), aio4c_thread_arg(writer));

    return writer;
}

void FreeWriter(Thread* parent, Writer** writer) {
    Writer* pWriter = NULL;
    int i = 0;

    if (writer != NULL && (pWriter = *writer) != NULL) {
        if (pWriter->thread != NULL) {
            ThreadCancel(pWriter->thread, true);
            FreeThread(&pWriter->thread);
        }

        if (pWriter->buffers != NULL) {
            for (i = 0; i < pWriter->maxConnections; i++) {
                if (pWriter->buffers[i] != NULL) {
                    FreeBuffer(&pWriter->buffers[i]);
                }
            }
            pWriter->maxConnections = 0;
            pWriter->numConnections = 0;
            free(pWriter->buffers);
            pWriter->buffers = NULL;
        }

        if (pWriter->connections != NULL) {
            free(pWriter->connections);
            pWriter->connections = NULL;
        }

        if (pWriter->condition != NULL) {
            FreeCondition(&pWriter->condition);
        }

        if (pWriter->polling != NULL) {
            free(pWriter->polling);
            pWriter->polling = NULL;
        }

        if (pWriter->lock != NULL) {
            FreeLock(&pWriter->lock);
        }

        free(pWriter);
        *writer = NULL;
    }
}

static void _WriterCloseHandler(Event event, Connection* source, Writer* writer) {
    int i = 0, j = 0;
    Buffer* buffer = NULL;

    Thread* currentThread = ThreadSelf();
    if (currentThread == NULL) {
        currentThread = writer->thread;
    }

    TakeLock(writer->lock);

    if (currentThread != writer->thread) {
        _WriterWakeUp(currentThread, writer);
    }

    for (i = 1; i < writer->numConnections; i++) {
        if (writer->connections[i] == source) {
            buffer = writer->buffers[i];

            for (j = i; j < writer->numConnections - 1; j++) {
                writer->connections[j] = writer->connections[j + 1];
                memcpy(&writer->polling[j], &writer->polling[j + 1], sizeof(aio4c_poll_t));
                writer->buffers[j] = writer->buffers[j + 1];
            }

            writer->connections[j] = NULL;
            memset(&writer->polling[j], 0, sizeof(aio4c_poll_t));
            writer->buffers[j] = BufferReset(buffer);

            writer->numConnections--;
            i--;
        }
    }

    ReleaseLock(writer->lock);
}

static void _WriterOutboundDataHandler(Event event, Connection* connection, Writer* writer) {
    Thread* current = ThreadSelf();

    TakeLock(writer->lock);

    _WriterWakeUp(current, writer);

    _WriterConnectionQueue(writer, connection, NULL);

    ConnectionAddSystemHandler(connection, CLOSE_EVENT, aio4c_connection_handler(_WriterCloseHandler), aio4c_connection_handler_arg(writer), true);

    NotifyCondition(writer->condition, writer->lock);

    ReleaseLock(writer->lock);
}

void WriterManageConnection(Writer* writer, Connection* connection) {
    ConnectionAddSystemHandler(connection, CONNECTING_EVENT, aio4c_connection_handler(_WriterOutboundDataHandler), aio4c_connection_handler_arg(writer), false);
    ConnectionAddSystemHandler(connection, OUTBOUND_DATA_EVENT, aio4c_connection_handler(_WriterOutboundDataHandler), aio4c_connection_handler_arg(writer), false);
}

