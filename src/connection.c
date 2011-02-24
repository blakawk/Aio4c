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
#include <aio4c/connection.h>

#include <aio4c/address.h>
#include <aio4c/alloc.h>
#include <aio4c/buffer.h>
#include <aio4c/event.h>
#include <aio4c/log.h>
#include <aio4c/stats.h>
#include <aio4c/thread.h>
#include <aio4c/types.h>

#include <errno.h>
#include <fcntl.h>
#include <poll.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <unistd.h>

char* ConnectionStateString[MAX_CONNECTION_STATES] = {
    "NONE",
    "INITIALIZED",
    "CONNECTING",
    "CONNECTED",
    "CLOSED"
};

Connection* NewConnection(aio4c_size_t bufferSize, Address* address) {
    Connection* connection = NULL;

    if ((connection = aio4c_malloc(sizeof(Connection))) == NULL) {
        return NULL;
    }

    if ((connection->readBuffer = NewBuffer(bufferSize)) == NULL) {
        FreeConnection(&connection);
        return NULL;
    }

    if ((connection->writeBuffer = NewBuffer(bufferSize)) == NULL) {
        FreeConnection(&connection);
        return NULL;
    }

    BufferLimit(connection->writeBuffer, 0);
    connection->dataBuffer = NULL;
    connection->socket = -1;
    connection->state = NONE;
    connection->systemHandlers = NewEventQueue();
    connection->userHandlers = NewEventQueue();
    connection->address = address;
    connection->closeCause = NO_ERROR;
    connection->closeCode = 0;
    connection->string = address->string;
    connection->lock = NewLock();
    connection->bufferSize = bufferSize;
    memset(connection->closedBy, 0, AIO4C_MAX_OWNERS * sizeof(aio4c_bool_t));
    connection->closedBy[ACCEPTOR] = true;
    connection->readKey = NULL;
    connection->writeKey = NULL;

    return connection;
}

Connection* NewConnectionFactory(aio4c_size_t bufferSize) {
    Connection* connection = NULL;

    if ((connection = aio4c_malloc(sizeof(Connection))) == NULL) {
        return NULL;
    }

    connection->socket = -1;
    connection->readBuffer = NULL;
    connection->dataBuffer = NULL;
    connection->writeBuffer = NULL;
    connection->state = CLOSED;
    connection->systemHandlers = NewEventQueue();
    connection->userHandlers = NewEventQueue();
    connection->address = NULL;
    connection->closeCause = NO_ERROR;
    connection->closeCode = 0;
    connection->string = "factory";
    connection->lock = NewLock();
    connection->bufferSize = bufferSize;
    memset(connection->closedBy, 0, AIO4C_MAX_OWNERS * sizeof(aio4c_bool_t));
    connection->readKey = NULL;
    connection->writeKey = NULL;

    return connection;
}

Connection* ConnectionFactoryCreate(Connection* factory, Address* address, aio4c_socket_t socket) {
    Connection* connection = NULL;

    connection = NewConnection(factory->bufferSize, address);

    connection->socket = socket;

    if (fcntl(connection->socket, F_SETFL, O_NONBLOCK) == -1) {
        shutdown(connection->socket, SHUT_RDWR);
        close(connection->socket);
        aio4c_free(connection);
        return NULL;
    }

    CopyEventQueue(connection->userHandlers, factory->userHandlers);
    CopyEventQueue(connection->systemHandlers, factory->systemHandlers);

    connection->closedBy[ACCEPTOR] = false;

    connection->state = CONNECTED;

    ConnectionEventHandle(connection, CONNECTED_EVENT, connection);

    return connection;
}

static Connection* _ConnectionHandleError(Connection* connection, ConnectionCloseCause cause, int closeCode) {
    connection->closeCause = cause;
    connection->closeCode = closeCode;

    switch(connection->closeCause) {
        case SOCKET_CREATION:
        case SOCKET_NONBLOCK:
        case GETSOCKOPT:
        case CONNECTING_ERROR:
        case READING:
        case WRITING:
            Log(ThreadSelf(), ERROR, "error on connection %s [%s]: %s", connection->string, ConnectionStateString[connection->state], strerror(connection->closeCode));
            break;
        case BUFFER_UNDERFLOW:
            Log(ThreadSelf(), ERROR, "buffer underflow on connection %s [%s] (buffer position = %d)", connection->string, ConnectionStateString[connection->state], connection->closeCode);
            break;
        case BUFFER_OVERFLOW:
            Log(ThreadSelf(), ERROR, "buffer overflow on connection %s [%s] (buffer position = %d)", connection->string, ConnectionStateString[connection->state], connection->closeCode);
            break;
        case INVALID_STATE:
            Log(ThreadSelf(), ERROR, "connection %s state %s invalid, expected %s", connection->string, ConnectionStateString[connection->state], ConnectionStateString[connection->closeCode]);
            break;
        case REMOTE_CLOSED:
            Log(ThreadSelf(), INFO, "connection %s remote closed", connection->string);
            break;
        case EVENT_HANDLER:
            Log(ThreadSelf(), ERROR, "cannot add event handler to connection %s", connection->string);
            break;
        case NO_ERROR:
            Log(ThreadSelf(), INFO, "connection %s closed", connection->string);
            break;
        case UNKNOWN:
            Log(ThreadSelf(), ERROR, "unknown error on connection %s", connection->string);
            break;
    }

    ReleaseLock(connection->lock);

    return ConnectionClose(connection);
}

Connection* ConnectionInit(Connection* connection) {
    TakeLock(connection->lock);

    if ((connection->socket = socket(PF_INET, SOCK_STREAM, 0)) == -1) {
        return _ConnectionHandleError(connection, SOCKET_CREATION, errno);
    }

    if (fcntl(connection->socket, F_SETFL, O_NONBLOCK) == -1) {
        return _ConnectionHandleError(connection, SOCKET_NONBLOCK, errno);
    }

    connection->state = INITIALIZED;

    ReleaseLock(connection->lock);

    ConnectionEventHandle(connection, INIT_EVENT, connection);

    return connection;
}

Connection* ConnectionConnect(Connection* connection) {
    Event eventToHandle = CONNECTING_EVENT;

    TakeLock(connection->lock);

    if (connection->state != INITIALIZED) {
        return _ConnectionHandleError(connection, INVALID_STATE, INITIALIZED);
    }

    if (connect(connection->socket, (struct sockaddr*)connection->address->address, sizeof(aio4c_addr_t)) == -1) {
        if (errno == EINPROGRESS) {
            connection->state = CONNECTING;
        } else {
            return _ConnectionHandleError(connection, CONNECTING_ERROR, errno);
        }
    } else {
        connection->state = CONNECTED;
        eventToHandle = CONNECTED_EVENT;
    }

    ReleaseLock(connection->lock);

    ConnectionEventHandle(connection, eventToHandle, connection);

    return connection;
}

Connection* ConnectionFinishConnect(Connection* connection) {
    int soError = 0;
    socklen_t soSize = sizeof(int);
    aio4c_poll_t polls[1] = { { .fd = connection->socket, .events = POLLOUT, .revents = 0 } };

    TakeLock(connection->lock);

    if (poll(polls, 1, -1) == -1) {
        return _ConnectionHandleError(connection, CONNECTING_ERROR, errno);
    }

    if (polls[0].revents > 0) {
        if (getsockopt(connection->socket, SOL_SOCKET, SO_ERROR, &soError, &soSize) != 0) {
            return _ConnectionHandleError(connection, GETSOCKOPT, errno);
        }

        if (soError != 0) {
            return _ConnectionHandleError(connection, CONNECTING_ERROR, soError);
        }

        connection->state = CONNECTED;

        ReleaseLock(connection->lock);

        ConnectionEventHandle(connection, CONNECTED_EVENT, connection);
    }

    return connection;
}

Connection* ConnectionRead(Connection* connection) {
    Buffer* buffer = NULL;
    ssize_t nbRead = 0;

    TakeLock(connection->lock);

    if (connection->state != CONNECTED && connection->state != CONNECTING) {
        return _ConnectionHandleError(connection, INVALID_STATE, CONNECTED);
    }

    buffer = connection->readBuffer;

    if (!BufferHasRemaining(buffer)) {
        return _ConnectionHandleError(connection, BUFFER_OVERFLOW, buffer->position);
    }

    if ((nbRead = read(connection->socket, &buffer->data[buffer->position], buffer->limit - buffer->position)) == -1) {
        return _ConnectionHandleError(connection, READING, errno);
    }

    ProbeSize(PROBE_NETWORK_READ_SIZE, nbRead);

    if (nbRead == 0) {
        return _ConnectionHandleError(connection, REMOTE_CLOSED, 0);
    }

    buffer->position += nbRead;

    ReleaseLock(connection->lock);

    ConnectionEventHandle(connection, READ_EVENT, connection);

    return connection;
}

Connection* ConnectionWrite(Connection* connection) {
    ssize_t nbWrite = 0;
    Buffer* buffer = connection->writeBuffer;

    TakeLock(connection->lock);

    if (connection->state != CONNECTED) {
        return _ConnectionHandleError(connection, INVALID_STATE, CONNECTED);
    }

    if (!BufferHasRemaining(buffer)) {
        BufferReset(buffer);
        ReleaseLock(connection->lock);
        ConnectionEventHandle(connection, WRITE_EVENT, connection);
        TakeLock(connection->lock);
        BufferFlip(buffer);
    }

    if (buffer->limit - buffer->position < 0) {
        return _ConnectionHandleError(connection, BUFFER_UNDERFLOW, buffer->position);
    }

    if ((nbWrite = write(connection->socket, &buffer->data[buffer->position], buffer->limit - buffer->position)) == -1) {
        return _ConnectionHandleError(connection, WRITING, errno);
    }

    ProbeSize(PROBE_NETWORK_WRITE_SIZE, nbWrite);

    buffer->position += nbWrite;

    ReleaseLock(connection->lock);

    return connection;
}

Connection* ConnectionClose(Connection* connection) {
    TakeLock(connection->lock);

    if (connection->state == CLOSED) {
        ReleaseLock(connection->lock);
        return connection;
    }

    Log(ThreadSelf(), DEBUG, "closing connection %s", connection->string);

    shutdown(connection->socket, SHUT_RDWR);

    connection->state = CLOSED;

    ReleaseLock(connection->lock);

    ConnectionEventHandle(connection, CLOSE_EVENT, connection);

    if (connection->closeCause == NO_ERROR) {
        _ConnectionHandleError(connection, NO_ERROR, 0);
    }

    return connection;
}

aio4c_bool_t ConnectionNoMoreUsed (Connection* connection, ConnectionOwner owner) {
    aio4c_bool_t result = true;
    int i = 0;

    TakeLock(connection->lock);

    connection->closedBy[owner] = true;

    for (i = 0; i < AIO4C_MAX_OWNERS && result; i++) {
        result = (result && connection->closedBy[i]);
    }

    if (result) {
        memset(connection->closedBy, 0, AIO4C_MAX_OWNERS * sizeof(aio4c_bool_t));
    }

    ReleaseLock(connection->lock);

    return result;
}

Connection* ConnectionAddHandler(Connection* connection, Event event, void (*handler)(Event,Connection*,void*), void* arg, aio4c_bool_t once) {
    EventHandler* eventHandler = NULL;

    eventHandler = NewEventHandler(event, aio4c_event_handler(handler), aio4c_event_handler_arg(arg), once);

    if (eventHandler != NULL) {
        if (EventHandlerAdd(connection->userHandlers, eventHandler) != NULL) {
            return connection;
        }
    }

    return _ConnectionHandleError(connection, EVENT_HANDLER, 0);
}

Connection* ConnectionAddSystemHandler(Connection* connection, Event event, void (*handler)(Event,Connection*,void*), void* arg, aio4c_bool_t once) {
    EventHandler* eventHandler = NULL;

    eventHandler = NewEventHandler(event, aio4c_event_handler(handler), aio4c_event_handler_arg(arg), once);

    if (eventHandler != NULL) {
        if (EventHandlerAdd(connection->systemHandlers, eventHandler) != NULL) {
            return connection;
        }
    }

    return _ConnectionHandleError(connection, EVENT_HANDLER, 0);
}

Connection* ConnectionEventHandle(Connection* connection, Event event, void* arg) {
    EventHandle(connection->systemHandlers, event, arg);
    EventHandle(connection->userHandlers, event, arg);

    return connection;
}

void FreeConnection(Connection** connection) {
    Connection* pConnection = NULL;

    if (connection != NULL && (pConnection = *connection) != NULL) {
        ConnectionClose(pConnection);

        if (pConnection->readBuffer != NULL) {
            FreeBuffer(&pConnection->readBuffer);
        }

        if (pConnection->writeBuffer != NULL) {
            FreeBuffer(&pConnection->writeBuffer);
        }

        if (pConnection->dataBuffer != NULL) {
            FreeBuffer(&pConnection->dataBuffer);
        }

        if (pConnection->address != NULL) {
            FreeAddress(&pConnection->address);
        }

        if (pConnection->userHandlers != NULL) {
            FreeEventQueue(&pConnection->userHandlers);
        }

        if (pConnection->systemHandlers != NULL) {
            FreeEventQueue(&pConnection->systemHandlers);
        }

        if (pConnection->lock != NULL) {
            FreeLock(&pConnection->lock);
        }

        aio4c_free(pConnection);
        *connection = NULL;
    }
}

