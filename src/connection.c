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
#include <aio4c/log.h>
#include <aio4c/stats.h>
#include <aio4c/thread.h>
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
    connection->dataFactory = NULL;

    return connection;
}

Connection* NewConnectionFactory(BufferPool* pool, void* (*dataFactory)(Connection*)) {
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
    connection->dataFactory = dataFactory;

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

    data = factory->dataFactory(connection);

    CopyEventQueue(connection->userHandlers, factory->userHandlers, data);
    CopyEventQueue(connection->systemHandlers, factory->systemHandlers, NULL);

    connection->closedBy[AIO4C_CONNECTION_OWNER_ACCEPTOR] = false;

    connection->state = AIO4C_CONNECTION_STATE_CONNECTED;

    ConnectionEventHandle(connection, AIO4C_CONNECTED_EVENT, connection);

    return connection;
}

#define _ConnectionHandleError(connection,level,error,code) \
    __ConnectionHandleError(__FILE__, __LINE__, connection, level, error, code)

static Connection* __ConnectionHandleError(char* file, int line, Connection* connection, LogLevel level, Error error, ErrorCode* code) {
    switch (error) {
        case AIO4C_BUFFER_OVERFLOW_ERROR:
        case AIO4C_BUFFER_UNDERFLOW_ERROR:
            _Raise(file, line, level, AIO4C_BUFFER_ERROR_TYPE, error, code);
            break;
        case AIO4C_CONNECTION_STATE_ERROR:
            _Raise(file, line, level, AIO4C_CONNECTION_STATE_ERROR_TYPE, error, code);
        default:
            code->connection = connection;
            _Raise(file, line, level, AIO4C_CONNECTION_ERROR_TYPE, error, code);
            break;
    }

    ReleaseLock(connection->lock);

    return ConnectionClose(connection);
}

Connection* ConnectionInit(Connection* connection) {
    ErrorCode code = AIO4C_ERROR_CODE_INITIALIZER;

    TakeLock(connection->lock);

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

    connection->state = AIO4C_CONNECTION_STATE_INITIALIZED;

    ReleaseLock(connection->lock);

    ConnectionEventHandle(connection, AIO4C_INIT_EVENT, connection);

    return connection;
}

Connection* ConnectionConnect(Connection* connection) {
    ErrorCode code = AIO4C_ERROR_CODE_INITIALIZER;
    Event eventToHandle = AIO4C_CONNECTING_EVENT;

    TakeLock(connection->lock);

    if (connection->state != AIO4C_CONNECTION_STATE_INITIALIZED) {
        return _ConnectionHandleError(connection, AIO4C_LOG_LEVEL_DEBUG, AIO4C_CONNECTION_STATE_ERROR, &code);
    }

    if (connect(connection->socket, (struct sockaddr*)connection->address->address, sizeof(aio4c_addr_t)) == -1) {
#ifndef AIO4C_WIN32
        if (errno == EINPROGRESS) {
            code.error = errno;
#else /* AIO4C_WIN32 */
        int error = WSAGetLastError();
        code.source = AIO4C_ERRNO_SOURCE_WSA;
        if (error == WSAEINPROGRESS || error == WSAEALREADY || error == WSAEWOULDBLOCK) {
#endif /* AIO4C_WIN32 */
            connection->state = AIO4C_CONNECTION_STATE_CONNECTING;
        } else {
            return _ConnectionHandleError(connection, AIO4C_LOG_LEVEL_ERROR, AIO4C_CONNECT_ERROR, &code);
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
    ErrorCode code = AIO4C_ERROR_CODE_INITIALIZER;
    int soError = 0;
    socklen_t soSize = sizeof(int);

    TakeLock(connection->lock);

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

        connection->state = AIO4C_CONNECTION_STATE_CONNECTED;

        ReleaseLock(connection->lock);

        ConnectionEventHandle(connection,AIO4C_CONNECTED_EVENT, connection);
    }

    return connection;
}

Connection* ConnectionRead(Connection* connection) {
    Buffer* buffer = NULL;
    ssize_t nbRead = 0;
    ErrorCode code = AIO4C_ERROR_CODE_INITIALIZER;

    TakeLock(connection->lock);

    if (connection->state != AIO4C_CONNECTION_STATE_CONNECTED && connection->state != AIO4C_CONNECTION_STATE_CONNECTING) {
        return _ConnectionHandleError(connection, AIO4C_LOG_LEVEL_DEBUG, AIO4C_CONNECTION_STATE_ERROR, &code);
    }

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
        return _ConnectionHandleError(connection, AIO4C_LOG_LEVEL_INFO, AIO4C_CONNECTION_DISCONNECTED, &code);
    }

    buffer->position += nbRead;

    ReleaseLock(connection->lock);

    ConnectionEventHandle(connection, AIO4C_READ_EVENT, connection);

    return connection;
}

Connection* ConnectionWrite(Connection* connection) {
    ssize_t nbWrite = 0;
    Buffer* buffer = connection->writeBuffer;
    ErrorCode code = AIO4C_ERROR_CODE_INITIALIZER;

    TakeLock(connection->lock);

    if (connection->state != AIO4C_CONNECTION_STATE_CONNECTED) {
        return _ConnectionHandleError(connection, AIO4C_LOG_LEVEL_DEBUG, AIO4C_CONNECTION_STATE_ERROR, &code);
    }

    if (!BufferHasRemaining(buffer)) {
        BufferReset(buffer);
        ReleaseLock(connection->lock);
        ConnectionEventHandle(connection, AIO4C_WRITE_EVENT, connection);
        TakeLock(connection->lock);
        BufferFlip(buffer);
    }

    if (buffer->limit - buffer->position < 0) {
        code.buffer = buffer;
        return _ConnectionHandleError(connection, AIO4C_LOG_LEVEL_ERROR, AIO4C_BUFFER_UNDERFLOW_ERROR, &code);
    }

    if ((nbWrite = send(connection->socket, (void*)&buffer->data[buffer->position], buffer->limit - buffer->position, 0)) < 0) {
#ifndef AIO4C_WIN32
        code.error = errno;
#else /* AIO4C_WIN32 */
        code.source = AIO4C_ERRNO_SOURCE_WSA;
#endif /* AIO4C_WIN32 */
        return _ConnectionHandleError(connection, AIO4C_LOG_LEVEL_ERROR, AIO4C_WRITE_ERROR, &code);
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
#else /* AIO4C_WIN32 */
    shutdown(connection->socket, SD_BOTH);
    closesocket(connection->socket);
#endif /* AIO4C_WIN32 */

    connection->state = AIO4C_CONNECTION_STATE_CLOSED;

    ReleaseLock(connection->lock);

    ConnectionEventHandle(connection, AIO4C_CLOSE_EVENT, connection);

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
    ErrorCode code = AIO4C_ERROR_CODE_INITIALIZER;

    TakeLock(connection->lock);

    eventHandler = NewEventHandler(event, aio4c_event_handler(handler), aio4c_event_handler_arg(arg), once);

    if (eventHandler != NULL) {
        if (EventHandlerAdd(connection->userHandlers, eventHandler) != NULL) {
            ReleaseLock(connection->lock);
            return connection;
        }
    }

    return _ConnectionHandleError(connection, AIO4C_LOG_LEVEL_ERROR, AIO4C_EVENT_HANDLER_ERROR, &code);
}

Connection* ConnectionAddSystemHandler(Connection* connection, Event event, void (*handler)(Event,Connection*,void*), void* arg, aio4c_bool_t once) {
    EventHandler* eventHandler = NULL;
    ErrorCode code = AIO4C_ERROR_CODE_INITIALIZER;

    TakeLock(connection->lock);

    eventHandler = NewEventHandler(event, aio4c_event_handler(handler), aio4c_event_handler_arg(arg), once);

    if (eventHandler != NULL) {
        if (EventHandlerAdd(connection->systemHandlers, eventHandler) != NULL) {
            ReleaseLock(connection->lock);
            return connection;
        }
    }

    return _ConnectionHandleError(connection, AIO4C_LOG_LEVEL_ERROR, AIO4C_EVENT_HANDLER_ERROR, &code);
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

