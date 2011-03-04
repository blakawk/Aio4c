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

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#ifndef AIO4C_WIN32

#include <fcntl.h>
#include <poll.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <unistd.h>
#include <errno.h>

#else

#include <winsock2.h>

#endif


char* ConnectionStateString[AIO4C_CONNECTION_STATE_MAX] = {
    "NONE",
    "INITIALIZED",
    "CONNECTING",
    "CONNECTED",
    "CLOSED"
};

Connection* NewConnection(BufferPool* pool, Address* address, aio4c_bool_t freeAddress) {
    Connection* connection = NULL;

    if ((connection = aio4c_malloc(sizeof(Connection))) == NULL) {
        return NULL;
    }


    connection->readBuffer = AllocateBuffer(pool);
    connection->writeBuffer = AllocateBuffer(pool);
    BufferLimit(connection->writeBuffer, 0);
    connection->dataBuffer = NULL;
    connection->socket = -1;
    connection->state = AIO4C_CONNECTION_STATE_NONE;
    connection->systemHandlers = NewEventQueue();
    connection->userHandlers = NewEventQueue();
    connection->address = address;
    connection->closeCause = AIO4C_CONNECTION_CLOSE_CAUSE_NO_ERROR;
    connection->closeCode = 0;
    connection->string = address->string;
    connection->lock = NewLock();
    memset(connection->closedBy, 0, AIO4C_CONNECTION_OWNER_MAX * sizeof(aio4c_bool_t));
    connection->closedBy[AIO4C_CONNECTION_OWNER_ACCEPTOR] = true;
    connection->readKey = NULL;
    connection->writeKey = NULL;
    connection->pool = NULL;
    connection->freeAddress = freeAddress;

    return connection;
}

Connection* NewConnectionFactory(BufferPool* pool) {
    Connection* connection = NULL;

    if ((connection = aio4c_malloc(sizeof(Connection))) == NULL) {
        return NULL;
    }

    connection->socket = -1;
    connection->readBuffer = NULL;
    connection->dataBuffer = NULL;
    connection->writeBuffer = NULL;
    connection->pool = pool;
    connection->state = AIO4C_CONNECTION_STATE_CLOSED;
    connection->systemHandlers = NewEventQueue();
    connection->userHandlers = NewEventQueue();
    connection->address = NULL;
    connection->closeCause = AIO4C_CONNECTION_CLOSE_CAUSE_NO_ERROR;
    connection->closeCode = 0;
    connection->string = "factory";
    connection->lock = NewLock();
    memset(connection->closedBy, 0, AIO4C_CONNECTION_OWNER_MAX * sizeof(aio4c_bool_t));
    connection->readKey = NULL;
    connection->writeKey = NULL;
    connection->freeAddress = false;

    return connection;
}

Connection* ConnectionFactoryCreate(Connection* factory, Address* address, aio4c_socket_t socket) {
    Connection* connection = NULL;

    connection = NewConnection(factory->pool, address, true);

    connection->socket = socket;

#ifdef AIO4C_WIN32
    unsigned long ioctl = 1;
    if (ioctlsocket(connection->socket, FIONBIO, &ioctl) != 0) {
        shutdown(connection->socket, SD_BOTH);
        closesocket(connection->socket);
#else
    if (fcntl(connection->socket, F_SETFL, O_NONBLOCK) == -1) {
        shutdown(connection->socket, SHUT_RDWR);
        close(connection->socket);
#endif
        aio4c_free(connection);
        return NULL;
    }

    CopyEventQueue(connection->userHandlers, factory->userHandlers);
    CopyEventQueue(connection->systemHandlers, factory->systemHandlers);

    connection->closedBy[AIO4C_CONNECTION_OWNER_ACCEPTOR] = false;

    connection->state = AIO4C_CONNECTION_STATE_CONNECTED;

    ConnectionEventHandle(connection, AIO4C_CONNECTED_EVENT, connection);

    return connection;
}

static Connection* _ConnectionHandleError(Connection* connection, ConnectionCloseCause cause, int closeCode) {
    connection->closeCause = cause;
    connection->closeCode = closeCode;

    switch(connection->closeCause) {
        case AIO4C_CONNECTION_CLOSE_CAUSE_SOCKET_CREATION:
        case AIO4C_CONNECTION_CLOSE_CAUSE_SOCKET_NONBLOCK:
        case AIO4C_CONNECTION_CLOSE_CAUSE_GETSOCKOPT:
        case AIO4C_CONNECTION_CLOSE_CAUSE_CONNECTING_ERROR:
        case AIO4C_CONNECTION_CLOSE_CAUSE_READING:
        case AIO4C_CONNECTION_CLOSE_CAUSE_WRITING:
            Log(ThreadSelf(), AIO4C_LOG_LEVEL_ERROR, "error on connection %s [%s]: %s", connection->string, ConnectionStateString[connection->state], strerror(connection->closeCode));
            break;
        case AIO4C_CONNECTION_CLOSE_CAUSE_BUFFER_UNDERFLOW:
            Log(ThreadSelf(), AIO4C_LOG_LEVEL_ERROR, "buffer underflow on connection %s [%s] (buffer position = %d)", connection->string, ConnectionStateString[connection->state], connection->closeCode);
            break;
        case AIO4C_CONNECTION_CLOSE_CAUSE_BUFFER_OVERFLOW:
            Log(ThreadSelf(), AIO4C_LOG_LEVEL_ERROR, "buffer overflow on connection %s [%s] (buffer position = %d)", connection->string, ConnectionStateString[connection->state], connection->closeCode);
            break;
        case AIO4C_CONNECTION_CLOSE_CAUSE_INVALID_STATE:
            Log(ThreadSelf(), AIO4C_LOG_LEVEL_ERROR, "connection %s state %s invalid, expected %s", connection->string, ConnectionStateString[connection->state], ConnectionStateString[connection->closeCode]);
            break;
        case AIO4C_CONNECTION_CLOSE_CAUSE_REMOTE_CLOSED:
            Log(ThreadSelf(), AIO4C_LOG_LEVEL_INFO, "connection %s remote closed", connection->string);
            break;
        case AIO4C_CONNECTION_CLOSE_CAUSE_EVENT_HANDLER:
            Log(ThreadSelf(), AIO4C_LOG_LEVEL_ERROR, "cannot add event handler to connection %s", connection->string);
            break;
        case AIO4C_CONNECTION_CLOSE_CAUSE_NO_ERROR:
            Log(ThreadSelf(), AIO4C_LOG_LEVEL_INFO, "connection %s closed", connection->string);
            break;
        case AIO4C_CONNECTION_CLOSE_CAUSE_UNKNOWN:
            Log(ThreadSelf(), AIO4C_LOG_LEVEL_ERROR, "unknown error on connection %s", connection->string);
            break;
        default:
            break;
    }

    ReleaseLock(connection->lock);

    return ConnectionClose(connection);
}

Connection* ConnectionInit(Connection* connection) {
    TakeLock(connection->lock);

    if ((connection->socket = socket(PF_INET, SOCK_STREAM, 0)) == -1) {
        return _ConnectionHandleError(connection, AIO4C_CONNECTION_CLOSE_CAUSE_SOCKET_CREATION, errno);
    }

#ifndef AIO4C_WIN32
    if (fcntl(connection->socket, F_SETFL, O_NONBLOCK) == -1) {
#else
    unsigned long ioctl = 1;
    if (ioctlsocket(connection->socket, FIONBIO, &ioctl) != 0) {
#endif
        return _ConnectionHandleError(connection, AIO4C_CONNECTION_CLOSE_CAUSE_SOCKET_NONBLOCK, errno);
    }

    connection->state = AIO4C_CONNECTION_STATE_INITIALIZED;

    ReleaseLock(connection->lock);

    ConnectionEventHandle(connection, AIO4C_INIT_EVENT, connection);

    return connection;
}

Connection* ConnectionConnect(Connection* connection) {
    Event eventToHandle = AIO4C_CONNECTING_EVENT;

    TakeLock(connection->lock);

    if (connection->state != AIO4C_CONNECTION_STATE_INITIALIZED) {
        return _ConnectionHandleError(connection, AIO4C_CONNECTION_CLOSE_CAUSE_INVALID_STATE, AIO4C_CONNECTION_STATE_INITIALIZED);
    }

    if (connect(connection->socket, (struct sockaddr*)connection->address->address, sizeof(aio4c_addr_t)) == -1) {
#ifdef AIO4C_WIN32
        int error = WSAGetLastError();
        if (error == WSAEINPROGRESS || error == WSAEALREADY) {
#else
        if (errno == EINPROGRESS) {
#endif
            connection->state = AIO4C_CONNECTION_STATE_CONNECTING;
        } else {
            return _ConnectionHandleError(connection, AIO4C_CONNECTION_CLOSE_CAUSE_CONNECTING_ERROR, errno);
        }
    } else {
        connection->state = AIO4C_CONNECTION_STATE_CONNECTED;
        eventToHandle = AIO4C_CONNECTED_EVENT;
    }

    ReleaseLock(connection->lock);

    ConnectionEventHandle(connection, eventToHandle, connection);

    return connection;
}

Connection* ConnectionFinishConnect(Connection* connection) {
    char soError = 0;
    socklen_t soSize = sizeof(char);
    aio4c_poll_t polls[1] = { { .fd = connection->socket, .events = POLLOUT, .revents = 0 } };

    TakeLock(connection->lock);

#ifndef AIO4C_WIN32
    if (poll(polls, 1, -1) == -1) {
#else
    if (WSAPoll(polls, 1, -1) == SOCKET_ERROR) {
#endif
        return _ConnectionHandleError(connection, AIO4C_CONNECTION_CLOSE_CAUSE_CONNECTING_ERROR, errno);
    }

    if (polls[0].revents > 0) {
        if (getsockopt(connection->socket, SOL_SOCKET, SO_ERROR, &soError, &soSize) != 0) {
            return _ConnectionHandleError(connection, AIO4C_CONNECTION_CLOSE_CAUSE_GETSOCKOPT, errno);
        }

        if (soError != 0) {
            return _ConnectionHandleError(connection, AIO4C_CONNECTION_CLOSE_CAUSE_CONNECTING_ERROR, soError);
        }

        connection->state = AIO4C_CONNECTION_STATE_CONNECTED;

        ReleaseLock(connection->lock);

        ConnectionEventHandle(connection,AIO4C_CONNECTED_EVENT, connection);
    }

    return connection;
}

Connection* ConnectionRead(Connection* connection) {
    Buffer* buffer = NULL;
    ssize_t nbRead = 0;

    TakeLock(connection->lock);

    if (connection->state != AIO4C_CONNECTION_STATE_CONNECTED && connection->state != AIO4C_CONNECTION_STATE_CONNECTING) {
        return _ConnectionHandleError(connection, AIO4C_CONNECTION_CLOSE_CAUSE_INVALID_STATE, AIO4C_CONNECTION_STATE_CONNECTED);
    }

    buffer = connection->readBuffer;

    if (!BufferHasRemaining(buffer)) {
        return _ConnectionHandleError(connection, AIO4C_CONNECTION_CLOSE_CAUSE_BUFFER_OVERFLOW, buffer->position);
    }

    if ((nbRead = recv(connection->socket, (void*)&buffer->data[buffer->position], buffer->limit - buffer->position, 0)) < 0) {
        return _ConnectionHandleError(connection, AIO4C_CONNECTION_CLOSE_CAUSE_READING, errno);
    }

    ProbeSize(AIO4C_PROBE_NETWORK_READ_SIZE, nbRead);

    if (nbRead == 0) {
        return _ConnectionHandleError(connection, AIO4C_CONNECTION_CLOSE_CAUSE_REMOTE_CLOSED, 0);
    }

    buffer->position += nbRead;

    ReleaseLock(connection->lock);

    ConnectionEventHandle(connection, AIO4C_READ_EVENT, connection);

    return connection;
}

Connection* ConnectionWrite(Connection* connection) {
    ssize_t nbWrite = 0;
    Buffer* buffer = connection->writeBuffer;

    TakeLock(connection->lock);

    if (connection->state != AIO4C_CONNECTION_STATE_CONNECTED) {
        return _ConnectionHandleError(connection, AIO4C_CONNECTION_CLOSE_CAUSE_INVALID_STATE, AIO4C_CONNECTION_STATE_CONNECTED);
    }

    if (!BufferHasRemaining(buffer)) {
        BufferReset(buffer);
        ReleaseLock(connection->lock);
        ConnectionEventHandle(connection, AIO4C_WRITE_EVENT, connection);
        TakeLock(connection->lock);
        BufferFlip(buffer);
    }

    if (buffer->limit - buffer->position < 0) {
        return _ConnectionHandleError(connection, AIO4C_CONNECTION_CLOSE_CAUSE_BUFFER_UNDERFLOW, buffer->position);
    }

    if ((nbWrite = send(connection->socket, (void*)&buffer->data[buffer->position], buffer->limit - buffer->position, 0)) < 0) {
        return _ConnectionHandleError(connection, AIO4C_CONNECTION_CLOSE_CAUSE_WRITING, errno);
    }

    ProbeSize(AIO4C_PROBE_NETWORK_WRITE_SIZE, nbWrite);

    buffer->position += nbWrite;

    ReleaseLock(connection->lock);

    return connection;
}

void EnableWriteInterest(Connection* connection) {
    ConnectionEventHandle(connection, AIO4C_OUTBOUND_DATA_EVENT, connection);
}

Connection* ConnectionClose(Connection* connection) {
    TakeLock(connection->lock);

    if (connection->state == AIO4C_CONNECTION_STATE_CLOSED) {
        ReleaseLock(connection->lock);
        return connection;
    }

    Log(ThreadSelf(), AIO4C_LOG_LEVEL_DEBUG, "closing connection %s", connection->string);

#ifndef AIO4C_WIN32
    shutdown(connection->socket, SHUT_RDWR);
    close(connection->socket);
#else
    shutdown(connection->socket, SD_BOTH);
    closesocket(connection->socket);
#endif

    connection->state = AIO4C_CONNECTION_STATE_CLOSED;

    ReleaseLock(connection->lock);

    ConnectionEventHandle(connection, AIO4C_CLOSE_EVENT, connection);

    if (connection->closeCause == AIO4C_CONNECTION_CLOSE_CAUSE_NO_ERROR) {
        _ConnectionHandleError(connection, AIO4C_CONNECTION_CLOSE_CAUSE_NO_ERROR, 0);
    }

    return connection;
}

aio4c_bool_t ConnectionNoMoreUsed (Connection* connection, ConnectionOwner owner) {
    aio4c_bool_t result = true;
    int i = 0;

    TakeLock(connection->lock);

    connection->closedBy[owner] = true;

    for (i = 0; i < AIO4C_CONNECTION_OWNER_MAX && result; i++) {
        result = (result && connection->closedBy[i]);
    }

    if (result) {
        memset(connection->closedBy, 0, AIO4C_CONNECTION_OWNER_MAX * sizeof(aio4c_bool_t));
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

    return _ConnectionHandleError(connection, AIO4C_CONNECTION_CLOSE_CAUSE_EVENT_HANDLER, 0);
}

Connection* ConnectionAddSystemHandler(Connection* connection, Event event, void (*handler)(Event,Connection*,void*), void* arg, aio4c_bool_t once) {
    EventHandler* eventHandler = NULL;

    eventHandler = NewEventHandler(event, aio4c_event_handler(handler), aio4c_event_handler_arg(arg), once);

    if (eventHandler != NULL) {
        if (EventHandlerAdd(connection->systemHandlers, eventHandler) != NULL) {
            return connection;
        }
    }

    return _ConnectionHandleError(connection, AIO4C_CONNECTION_CLOSE_CAUSE_EVENT_HANDLER, 0);
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
            ReleaseBuffer(&pConnection->readBuffer);
        }

        if (pConnection->writeBuffer != NULL) {
            ReleaseBuffer(&pConnection->writeBuffer);
        }

        if (pConnection->freeAddress && pConnection->address != NULL) {
            FreeAddress(&pConnection->address);
        }

        if (pConnection->dataBuffer != NULL) {
            ReleaseBuffer(&pConnection->dataBuffer);
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

