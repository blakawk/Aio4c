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
#include <aio4c/error.h>
#include <aio4c/event.h>
#include <aio4c/lock.h>
#include <aio4c/log.h>
#include <aio4c/stats.h>
#include <aio4c/types.h>

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#ifndef AIO4C_WIN32
#include <fcntl.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <unistd.h>
#include <errno.h>
#ifdef AIO4C_HAVE_POLL
#include <poll.h>
#else /* AIO4C_HAVE_POLL */
#include <sys/select.h>
#endif /* AIO4C_HAVE_POLL */
#else /* AIO4C_WIN32 */
#include <winsock2.h>
#endif /* AIO4C_WIN32 */

char* ConnectionStateString[AIO4C_CONNECTION_STATE_MAX] = {
    "NONE",
    "INITIALIZED",
    "CONNECTING",
    "CONNECTED",
    "PENDING CLOSE",
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
    connection->stateLock = NewLock();
    connection->systemHandlers = NewEventQueue();
    connection->userHandlers = NewEventQueue();
    connection->address = address;
    connection->closedForError = false;
    connection->string = address->string;
    memset(connection->closedBy, 0, AIO4C_CONNECTION_OWNER_MAX * sizeof(aio4c_bool_t));
    connection->closedBy[AIO4C_CONNECTION_OWNER_ACCEPTOR] = true;
    connection->closedByLock = NewLock();
    memset(connection->managedBy, 0, AIO4C_CONNECTION_OWNER_MAX * sizeof(aio4c_bool_t));
    connection->managedBy[AIO4C_CONNECTION_OWNER_ACCEPTOR] = true;
    connection->managedBy[AIO4C_CONNECTION_OWNER_CLIENT] = true;
    connection->managedByLock = NewLock();
    connection->readKey = NULL;
    connection->writeKey = NULL;
    connection->pool = NULL;
    connection->freeAddress = freeAddress;
    connection->dataFactory = NULL;
    connection->dataFactoryArg = NULL;
    connection->canRead = false;
    connection->canWrite = false;

    return connection;
}

Connection* NewConnectionFactory(BufferPool* pool, void* (*dataFactory)(Connection*,void*), void* dataFactoryArg) {
    Connection* connection = NULL;

    if ((connection = aio4c_malloc(sizeof(Connection))) == NULL) {
        return NULL;
    }

    connection->socket = -1;
    connection->readBuffer = NULL;
    connection->dataBuffer = NULL;
    connection->writeBuffer = NULL;
    connection->pool = pool;
    connection->state = AIO4C_CONNECTION_STATE_NONE;
    connection->stateLock = NULL;
    connection->systemHandlers = NewEventQueue();
    connection->userHandlers = NewEventQueue();
    connection->address = NULL;
    connection->closedForError = false;
    connection->string = "factory";
    memset(connection->closedBy, 0, AIO4C_CONNECTION_OWNER_MAX * sizeof(aio4c_bool_t));
    connection->closedByLock = NULL;
    memset(connection->managedBy, 0, AIO4C_CONNECTION_OWNER_MAX * sizeof(aio4c_bool_t));
    connection->managedByLock = NULL;
    connection->readKey = NULL;
    connection->writeKey = NULL;
    connection->freeAddress = false;
    connection->dataFactory = dataFactory;
    connection->dataFactoryArg = dataFactoryArg;
    connection->canRead = false;
    connection->canWrite = false;

    return connection;
}

Connection* ConnectionFactoryCreate(Connection* factory, Address* address, aio4c_socket_t socket) {
    ErrorCode code = AIO4C_ERROR_CODE_INITIALIZER;
    Connection* connection = NULL;
    void* data = NULL;

    connection = NewConnection(factory->pool, address, true);

    connection->socket = socket;

#ifndef AIO4C_WIN32
    if (fcntl(connection->socket, F_SETFL, O_NONBLOCK) == -1) {
        code.error = errno;
        code.connection = connection;
        Raise(AIO4C_LOG_LEVEL_ERROR, AIO4C_CONNECTION_ERROR_TYPE, AIO4C_FCNTL_ERROR, &code);
        shutdown(connection->socket, SHUT_RDWR);
        close(connection->socket);
#else /* AIO4C_WIN32 */
    unsigned long ioctl = 1;
    if (ioctlsocket(connection->socket, FIONBIO, &ioctl) != 0) {
        code.source = AIO4C_ERRNO_SOURCE_WSA;
        code.connection = connection;
        Raise(AIO4C_LOG_LEVEL_ERROR, AIO4C_CONNECTION_ERROR_TYPE, AIO4C_FCNTL_ERROR, &code);
        shutdown(connection->socket, SD_BOTH);
        closesocket(connection->socket);
#endif /* AIO4C_WIN32 */
        aio4c_free(connection);
        return NULL;
    }

    data = factory->dataFactory(connection, factory->dataFactoryArg);

    CopyEventQueue(connection->userHandlers, factory->userHandlers, data);
    CopyEventQueue(connection->systemHandlers, factory->systemHandlers, NULL);

    connection->closedBy[AIO4C_CONNECTION_OWNER_ACCEPTOR] = false;
    connection->closedBy[AIO4C_CONNECTION_OWNER_CLIENT] = true;

    return connection;
}

static void _ConnectionEventHandle(Connection* connection, Event event) {
    EventHandle(connection->systemHandlers, event, connection);
    EventHandle(connection->userHandlers, event, connection);
}

void _ConnectionState(char* file, int line, Connection* connection, ConnectionState state) {
    TakeLock(connection->stateLock);

    if (connection->state == state) {
        Log(AIO4C_LOG_LEVEL_DEBUG, "%s:%d: connection %s already in state %s", file, line,
                connection->string, ConnectionStateString[state]);
        ReleaseLock(connection->stateLock);
        return;
    }

    Log(AIO4C_LOG_LEVEL_DEBUG, "%s:%d: connection %s [%s] -> [%s]", file, line, connection->string,
           ConnectionStateString[connection->state], ConnectionStateString[state]);

    connection->state = state;

    switch (state) {
        case AIO4C_CONNECTION_STATE_NONE:
            break;
        case AIO4C_CONNECTION_STATE_INITIALIZED:
            _ConnectionEventHandle(connection, AIO4C_INIT_EVENT);
            break;
        case AIO4C_CONNECTION_STATE_CONNECTING:
            connection->canRead = false;
            connection->canWrite = false;
            _ConnectionEventHandle(connection, AIO4C_CONNECTING_EVENT);
            break;
        case AIO4C_CONNECTION_STATE_CONNECTED:
            connection->canRead = true;
            connection->canWrite = true;
            _ConnectionEventHandle(connection, AIO4C_CONNECTED_EVENT);
            break;
        case AIO4C_CONNECTION_STATE_PENDING_CLOSE:
            connection->canRead = false;
            connection->canWrite = true;
            _ConnectionEventHandle(connection, AIO4C_PENDING_CLOSE_EVENT);
            break;
        case AIO4C_CONNECTION_STATE_CLOSED:
            connection->canRead = false;
            connection->canWrite = false;
            _ConnectionEventHandle(connection, AIO4C_CLOSE_EVENT);
            break;
        case AIO4C_CONNECTION_STATE_MAX:
            break;
    }

    ReleaseLock(connection->stateLock);
}

#define _ConnectionHandleError(connection,level,error,code) \
    __ConnectionHandleError(__FILE__, __LINE__, connection, level, error, code)
static Connection* __ConnectionHandleError(char* file, int line, Connection* connection, LogLevel level, Error error, ErrorCode* code) {
    connection->closedForError = true;

    switch (error) {
        case AIO4C_BUFFER_OVERFLOW_ERROR:
        case AIO4C_BUFFER_UNDERFLOW_ERROR:
            _Raise(file, line, level, AIO4C_BUFFER_ERROR_TYPE, error, code);
            break;
        case AIO4C_CONNECTION_STATE_ERROR:
            code->connection = connection;
            _Raise(file, line, level, AIO4C_CONNECTION_STATE_ERROR_TYPE, error, code);
            break;
        case AIO4C_CONNECTION_DISCONNECTED:
            connection->closedForError = false;
        default:
            code->connection = connection;
            _Raise(file, line, level, AIO4C_CONNECTION_ERROR_TYPE, error, code);
            break;
    }

    return ConnectionClose(connection, true);
}

Connection* ConnectionInit(Connection* connection) {
    ErrorCode code = AIO4C_ERROR_CODE_INITIALIZER;

#ifndef AIO4C_WIN32
    if ((connection->socket = socket(PF_INET, SOCK_STREAM, 0)) == -1) {
        code.error = errno;
#else /* AIO4C_WIN32 */
    if ((connection->socket = socket(PF_INET, SOCK_STREAM, 0)) == SOCKET_ERROR) {
        code.source = AIO4C_ERRNO_SOURCE_WSA;
#endif /* AIO4C_WIN32 */
        return _ConnectionHandleError(connection, AIO4C_LOG_LEVEL_ERROR, AIO4C_SOCKET_ERROR, &code);
    }

#ifndef AIO4C_WIN32
    if (fcntl(connection->socket, F_SETFL, O_NONBLOCK) == -1) {
        code.error = errno;
#else /* AIO4C_WIN32 */
    unsigned long ioctl = 1;
    if (ioctlsocket(connection->socket, FIONBIO, &ioctl) == SOCKET_ERROR) {
        code.source = AIO4C_ERRNO_SOURCE_WSA;
#endif /* AIO4C_WIN32 */
        return _ConnectionHandleError(connection, AIO4C_LOG_LEVEL_ERROR, AIO4C_FCNTL_ERROR, &code);
    }

    ConnectionState(connection, AIO4C_CONNECTION_STATE_INITIALIZED);

    return connection;
}

Connection* ConnectionConnect(Connection* connection) {
    ErrorCode code = AIO4C_ERROR_CODE_INITIALIZER;
    ConnectionState newState = AIO4C_CONNECTION_STATE_CONNECTING;

    if (connection->state != AIO4C_CONNECTION_STATE_INITIALIZED) {
        code.expected = AIO4C_CONNECTION_STATE_INITIALIZED;
        return _ConnectionHandleError(connection, AIO4C_LOG_LEVEL_DEBUG, AIO4C_CONNECTION_STATE_ERROR, &code);
    }

    if (connect(connection->socket, (struct sockaddr*)connection->address->address, sizeof(aio4c_addr_t)) == -1) {
#ifndef AIO4C_WIN32
        code.error = errno;
        if (errno == EINPROGRESS) {
#else /* AIO4C_WIN32 */
        int error = WSAGetLastError();
        code.source = AIO4C_ERRNO_SOURCE_WSA;
        if (error == WSAEINPROGRESS || error == WSAEALREADY || error == WSAEWOULDBLOCK) {
#endif /* AIO4C_WIN32 */
            newState = AIO4C_CONNECTION_STATE_CONNECTING;
        } else {
            return _ConnectionHandleError(connection, AIO4C_LOG_LEVEL_ERROR, AIO4C_CONNECT_ERROR, &code);
        }
    } else {
        newState = AIO4C_CONNECTION_STATE_CONNECTED;
    }

    ConnectionState(connection, newState);

    return connection;
}

Connection* ConnectionFinishConnect(Connection* connection) {
    ErrorCode code = AIO4C_ERROR_CODE_INITIALIZER;
    int soError = 0;
    socklen_t soSize = sizeof(int);

#ifdef AIO4C_HAVE_POLL
    aio4c_poll_t polls[1] = { { .fd = connection->socket, .events = POLLOUT, .revents = 0 } };

#ifndef AIO4C_WIN32
    if (poll(polls, 1, -1) == -1) {
        code.error = errno;
#else /* AIO4C_WIN32 */
    if (WSAPoll(polls, 1, -1) == SOCKET_ERROR) {
        code.source = AIO4C_ERRNO_SOURCE_WSA;
#endif /* AIO4C_WIN32 */
        return _ConnectionHandleError(connection, AIO4C_LOG_LEVEL_ERROR, AIO4C_POLL_ERROR, &code);
    }

    if (polls[0].revents > 0) {
#else /* AIO4C_HAVE_POLL */
    fd_set writeSet;
    fd_set errorSet;

    FD_ZERO(&writeSet);
    FD_ZERO(&errorSet);
    FD_SET(connection->socket, &writeSet);
    FD_SET(connection->socket, &errorSet);

#ifndef AIO4C_WIN32
    if (select(connection->socket + 1, NULL, &writeSet, &errorSet, NULL) == -1) {
        code.error = errno;
#else /* AIO4C_WIN32 */
    if (select(connection->socket + 1, NULL, &writeSet, &errorSet, NULL) == SOCKET_ERROR) {
        code.source = AIO4C_ERRNO_SOURCE_WSA;
#endif /* AIO4C_WIN32 */
        return _ConnectionHandleError(connection, AIO4C_LOG_LEVEL_ERROR, AIO4C_SELECT_ERROR, &code);
    }

    if (FD_ISSET(connection->socket, &writeSet) || FD_ISSET(connection->socket, &errorSet)) {
#endif /* AIO4C_HAVE_POLL */

#ifndef AIO4C_WIN32
        if (getsockopt(connection->socket, SOL_SOCKET, SO_ERROR, &soError, &soSize) != 0) {
            code.error = errno;
#else /* AI4OC_WIN32 */
        if (getsockopt(connection->socket, SOL_SOCKET, SO_ERROR, (char*)&soError, &soSize) == SOCKET_ERROR) {
            code.source = AIO4C_ERRNO_SOURCE_WSA;
#endif /* AIO4C_WIN32 */
            return _ConnectionHandleError(connection, AIO4C_LOG_LEVEL_ERROR, AIO4C_GETSOCKOPT_ERROR, &code);
        }

        if (soError != 0) {
#ifndef AIO4C_WIN32
            code.error = soError;
#else /* AIO4C_WIN32 */
            code.source = AIO4C_ERRNO_SOURCE_SOE;
            code.soError = soError;
#endif /* AIO4C_WIN32 */
            return _ConnectionHandleError(connection, AIO4C_LOG_LEVEL_ERROR, AIO4C_FINISH_CONNECT_ERROR, &code);
        }
    }

    return connection;
}

Connection* ConnectionRead(Connection* connection) {
    Buffer* buffer = NULL;
    ssize_t nbRead = 0;
    ErrorCode code = AIO4C_ERROR_CODE_INITIALIZER;

    buffer = connection->readBuffer;

    if (!BufferHasRemaining(buffer)) {
        code.buffer = buffer;
        return _ConnectionHandleError(connection, AIO4C_LOG_LEVEL_ERROR, AIO4C_BUFFER_OVERFLOW_ERROR, &code);
    }

    if ((nbRead = recv(connection->socket, (void*)&buffer->data[buffer->position], buffer->limit - buffer->position, 0)) < 0) {
#ifndef AIO4C_WIN32
        code.error = errno;
#else /* AIO4C_WIN32 */
        code.source = AIO4C_ERRNO_SOURCE_WSA;
#endif /* AIO4C_WIN32 */
        return _ConnectionHandleError(connection, AIO4C_LOG_LEVEL_ERROR, AIO4C_READ_ERROR, &code);
    }

    ProbeSize(AIO4C_PROBE_NETWORK_READ_SIZE, nbRead);

    if (nbRead == 0) {
        if (connection->state == AIO4C_CONNECTION_STATE_PENDING_CLOSE) {
            ConnectionState(connection, AIO4C_CONNECTION_STATE_CLOSED);
            return connection;
        } else {
            return _ConnectionHandleError(connection, AIO4C_LOG_LEVEL_INFO, AIO4C_CONNECTION_DISCONNECTED, &code);
        }
    }

    if (!connection->canRead) {
        BufferReset(buffer);
        return connection;
    }

    buffer->position += nbRead;

    _ConnectionEventHandle(connection, AIO4C_READ_EVENT);

    return connection;
}

Connection* ConnectionProcessData(Connection* connection) {
    _ConnectionEventHandle(connection, AIO4C_INBOUND_DATA_EVENT);
    return connection;
}

Connection* ConnectionShutdown(Connection* connection) {
    Log(AIO4C_LOG_LEVEL_DEBUG, "shutting down writing end on connection %s", connection->string);
#ifndef AIO4C_WIN32
    shutdown(connection->socket, SHUT_WR);
#else /* AIO4C_WIN32 */
    shutdown(connection->socket, SD_SEND);
#endif /* AIO4C_WIN32 */
    return connection;
}

aio4c_bool_t ConnectionWrite(Connection* connection) {
    ssize_t nbWrite = 0;
    Buffer* buffer = connection->writeBuffer;
    ErrorCode code = AIO4C_ERROR_CODE_INITIALIZER;
    aio4c_bool_t pendingCloseMemorized = false;

    if (!connection->canWrite) {
        code.expected = AIO4C_CONNECTION_STATE_CONNECTED;
        _ConnectionHandleError(connection, AIO4C_LOG_LEVEL_DEBUG, AIO4C_CONNECTION_STATE_ERROR, &code);
        return false;
    }

    if (connection->state == AIO4C_CONNECTION_STATE_PENDING_CLOSE) {
        pendingCloseMemorized = true;
    }

    if (!BufferHasRemaining(buffer)) {
        BufferReset(buffer);
        _ConnectionEventHandle(connection, AIO4C_WRITE_EVENT);
        BufferFlip(buffer);
    }

    if (buffer->limit - buffer->position < 0) {
        code.buffer = buffer;
        _ConnectionHandleError(connection, AIO4C_LOG_LEVEL_ERROR, AIO4C_BUFFER_UNDERFLOW_ERROR, &code);
        return false;
    }

    if ((nbWrite = send(connection->socket, (void*)&buffer->data[buffer->position], buffer->limit - buffer->position, 0)) < 0) {
#ifndef AIO4C_WIN32
        code.error = errno;
#else /* AIO4C_WIN32 */
        code.source = AIO4C_ERRNO_SOURCE_WSA;
#endif /* AIO4C_WIN32 */
        _ConnectionHandleError(connection, AIO4C_LOG_LEVEL_ERROR, AIO4C_WRITE_ERROR, &code);
        return false;
    }

    ProbeSize(AIO4C_PROBE_NETWORK_WRITE_SIZE, nbWrite);

    buffer->position += nbWrite;

    if (BufferHasRemaining(connection->writeBuffer)) {
        return true;
    } else if (pendingCloseMemorized) {
        ConnectionShutdown(connection);
    }

    return false;
}

void EnableWriteInterest(Connection* connection) {
    Log(AIO4C_LOG_LEVEL_DEBUG, "write interest for connection %s", connection->string);

    _ConnectionEventHandle(connection, AIO4C_OUTBOUND_DATA_EVENT);
}

Connection* ConnectionClose(Connection* connection, aio4c_bool_t force) {
    Log(AIO4C_LOG_LEVEL_DEBUG, "closing connection %s (force: %s)", connection->string, (force?"true":"false"));

    if (force) {
        ConnectionState(connection, AIO4C_CONNECTION_STATE_CLOSED);
    } else {
        ConnectionState(connection, AIO4C_CONNECTION_STATE_PENDING_CLOSE);
    }

    return connection;
}

aio4c_bool_t ConnectionNoMoreUsed (Connection* connection, ConnectionOwner owner) {
    aio4c_bool_t result = true;
    int i = 0;

    TakeLock(connection->closedByLock);

    connection->closedBy[owner] = true;

    Log(AIO4C_LOG_LEVEL_DEBUG, "connection %s closed by: [reader:%u,worker:%u,writer:%u,acceptor:%u,client:%u]", connection->string,
            connection->closedBy[AIO4C_CONNECTION_OWNER_READER], connection->closedBy[AIO4C_CONNECTION_OWNER_WORKER],
            connection->closedBy[AIO4C_CONNECTION_OWNER_WRITER], connection->closedBy[AIO4C_CONNECTION_OWNER_ACCEPTOR],
            connection->closedBy[AIO4C_CONNECTION_OWNER_CLIENT]);

    for (i = 0; i < AIO4C_CONNECTION_OWNER_MAX && result; i++) {
        result = (result && connection->closedBy[i]);
    }

    if (result) {
        memset(connection->closedBy, 0, AIO4C_CONNECTION_OWNER_MAX * sizeof(aio4c_bool_t));
    }

    ReleaseLock(connection->closedByLock);

    return result;
}

void ConnectionManagedBy(Connection* connection, ConnectionOwner owner) {
    aio4c_bool_t managedByAll = true;
    int i = 0;

    TakeLock(connection->managedByLock);

    connection->managedBy[owner] = true;

    Log(AIO4C_LOG_LEVEL_DEBUG, "connection %s managed by: [reader:%u,worker:%u,writer:%u]", connection->string,
        connection->managedBy[AIO4C_CONNECTION_OWNER_READER], connection->managedBy[AIO4C_CONNECTION_OWNER_WORKER],
        connection->managedBy[AIO4C_CONNECTION_OWNER_WRITER]);

    for (i = 0; i < AIO4C_CONNECTION_OWNER_MAX; i++) {
        managedByAll = (managedByAll && connection->managedBy[i]);
    }

    ReleaseLock(connection->managedByLock);

    if (managedByAll) {
        Log(AIO4C_LOG_LEVEL_DEBUG, "connection %s is managed by all threads", connection->string);
        ConnectionState(connection, AIO4C_CONNECTION_STATE_CONNECTED);
    }

}

Connection* ConnectionAddHandler(Connection* connection, Event event, void (*handler)(Event,Connection*,void*), void* arg, aio4c_bool_t once) {
    EventHandler* eventHandler = NULL;
    ErrorCode code = AIO4C_ERROR_CODE_INITIALIZER;

    eventHandler = NewEventHandler(event, aio4c_event_handler(handler), aio4c_event_handler_arg(arg), once);

    if (eventHandler != NULL) {
        if (EventHandlerAdd(connection->userHandlers, eventHandler) != NULL) {
            return connection;
        }
    }

    return _ConnectionHandleError(connection, AIO4C_LOG_LEVEL_ERROR, AIO4C_EVENT_HANDLER_ERROR, &code);
}

Connection* ConnectionAddSystemHandler(Connection* connection, Event event, void (*handler)(Event,Connection*,void*), void* arg, aio4c_bool_t once) {
    EventHandler* eventHandler = NULL;
    ErrorCode code = AIO4C_ERROR_CODE_INITIALIZER;

    eventHandler = NewEventHandler(event, aio4c_event_handler(handler), aio4c_event_handler_arg(arg), once);

    if (eventHandler != NULL) {
        if (EventHandlerAdd(connection->systemHandlers, eventHandler) != NULL) {
            return connection;
        }
    }

    return _ConnectionHandleError(connection, AIO4C_LOG_LEVEL_ERROR, AIO4C_EVENT_HANDLER_ERROR, &code);
}

Buffer* ConnectionGetReadBuffer(Connection* connection) {
    return connection->dataBuffer;
}

Buffer* ConnectionGetWriteBuffer(Connection* connection) {
    return connection->writeBuffer;
}

char* ConnectionGetString(Connection* connection) {
    return connection->string;
}

void FreeConnection(Connection** connection) {
    Connection* pConnection = NULL;

    if (connection != NULL && (pConnection = *connection) != NULL) {
        if (pConnection->socket != -1) {
#ifndef AIO4C_WIN32
            close(pConnection->socket);
#else /* AIO4C_WIN32 */
            closesocket(pConnection->socket);
#endif /* AIO4C_WIN32 */
        }

        _ConnectionEventHandle(pConnection, AIO4C_FREE_EVENT);

        if (pConnection->readBuffer != NULL) {
            ReleaseBuffer(&pConnection->readBuffer);
        }

        if (pConnection->writeBuffer != NULL) {
            ReleaseBuffer(&pConnection->writeBuffer);
        }

        if (pConnection->freeAddress && pConnection->address != NULL) {
            FreeAddress(&pConnection->address);
        }

        if (pConnection->userHandlers != NULL) {
            FreeEventQueue(&pConnection->userHandlers);
        }

        if (pConnection->systemHandlers != NULL) {
            FreeEventQueue(&pConnection->systemHandlers);
        }

        if (pConnection->managedByLock != NULL) {
            FreeLock(&pConnection->managedByLock);
        }

        if (pConnection->stateLock != NULL) {
            FreeLock(&pConnection->stateLock);
        }

        if (pConnection->closedByLock != NULL) {
            FreeLock(&pConnection->closedByLock);
        }

        aio4c_free(pConnection);
        *connection = NULL;
    }
}
