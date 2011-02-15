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
#include <aio4c/buffer.h>
#include <aio4c/event.h>
#include <aio4c/types.h>

#include <errno.h>
#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <unistd.h>

Connection* NewConnection(aio4c_size_t bufferSize, Address* address) {
    Connection* connection = NULL;

    if ((connection = malloc(sizeof(Connection))) == NULL) {
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

    connection->socket = -1;
    connection->state = NONE;
    connection->handlers = NewEventQueue();
    connection->address = address;
    connection->closeCause = NO_ERROR;
    connection->closeCode = 0;
    connection->string = address->string;

    return connection;
}

static Connection* _ConnectionHandleError(Connection* connection, ConnectionCloseCause cause, int closeCode) {
    connection->closeCause = cause;
    connection->closeCode = closeCode;

    return ConnectionClose(connection);
}

Connection* ConnectionInit(Connection* connection) {
    if ((connection->socket = socket(PF_INET, SOCK_STREAM, 0)) == -1) {
        return _ConnectionHandleError(connection, SOCKET_CREATION, errno);
    }

    if (fcntl(connection->socket, F_SETFL, O_NONBLOCK) == -1) {
        return _ConnectionHandleError(connection, SOCKET_NONBLOCK, errno);
    }

    connection->state = INITIALIZED;

    EventHandle(connection->handlers, INIT_EVENT, connection);

    return connection;
}

Connection* ConnectionConnect(Connection* connection) {
    Event eventToHandle = CONNECTING_EVENT;

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

    EventHandle(connection->handlers, eventToHandle, connection);

    return connection;
}

Connection* ConnectionFinishConnect(Connection* connection) {
    int soError = 0;
    socklen_t soSize = sizeof(int);
    if (getsockopt(connection->socket, SOL_SOCKET, SO_ERROR, &soError, &soSize) != 0) {
        return _ConnectionHandleError(connection, GETSOCKOPT, errno);
    }

    if (soError != 0) {
        return _ConnectionHandleError(connection, CONNECTING_ERROR, errno);
    }

    connection->state = CONNECTED;
    EventHandle(connection->handlers, CONNECTED_EVENT, connection);

    return connection;
}

Connection* ConnectionRead(Connection* connection) {
    Buffer* buffer = NULL;
    ssize_t nbRead = 0;

    if (connection->state != CONNECTED) {
        return _ConnectionHandleError(connection, INVALID_STATE, CONNECTED);
    }

    buffer = connection->readBuffer;

    if (buffer->limit - buffer->position <= 0) {
        return _ConnectionHandleError(connection, BUFFER_OVERFLOW, buffer->position);
    }

    if ((nbRead = read(connection->socket, &buffer->data[buffer->position], buffer->limit - buffer->position)) == -1) {
        return _ConnectionHandleError(connection, READING, errno);
    }

    if (nbRead == 0) {
        return _ConnectionHandleError(connection, REMOTE_CLOSED, 0);
    }

    buffer->position += nbRead;

    EventHandle(connection->handlers, READ_EVENT, connection);

    return connection;
}

Connection* ConnectionClose(Connection* connection) {
    if (connection->state == CLOSED) {
        return connection;
    }

    shutdown(connection->socket, SHUT_RDWR);

    connection->state = CLOSED;

    EventHandle(connection->handlers, CLOSE_EVENT, connection);

    return connection;
}

Connection* ConnectionAddHandler(Connection* connection, Event event, void (*handler)(Event,Connection*,void*), void* arg, aio4c_bool_t once) {
    EventHandler* eventHandler = NULL;

    eventHandler = NewEventHandler(event, aio4c_event_handler(handler), aio4c_event_handler_arg(arg), once);

    if (eventHandler != NULL) {
        if (EventHandlerAdd(connection->handlers, eventHandler) != NULL) {
            return connection;
        }
    }

    return _ConnectionHandleError(connection, EVENT_HANDLER, 0);
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

        if (pConnection->address != NULL) {
            FreeAddress(&pConnection->address);
        }

        if (pConnection->handlers != NULL) {
            FreeEventQueue(&pConnection->handlers);
        }

        free(pConnection);
        *connection = NULL;
    }
}

