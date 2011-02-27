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
#ifndef __AIO4C_CONNECTION_H__
#define __AIO4C_CONNECTION_H__

#include <aio4c/address.h>
#include <aio4c/buffer.h>
#include <aio4c/event.h>
#include <aio4c/thread.h>
#include <aio4c/types.h>

typedef enum e_ConnectionState {
    NONE = 0,
    INITIALIZED = 1,
    CONNECTING = 2,
    CONNECTED = 3,
    CLOSED = 4,
    MAX_CONNECTION_STATES = 5
} ConnectionState;

extern char* ConnectionStateString[MAX_CONNECTION_STATES];

typedef enum e_ConnectionCloseCauseType {
    UNKNOWN = 0,
    INVALID_STATE = 1,
    SOCKET_CREATION = 2,
    SOCKET_NONBLOCK = 3,
    GETSOCKOPT = 4,
    CONNECTING_ERROR = 5,
    BUFFER_UNDERFLOW = 6,
    BUFFER_OVERFLOW = 7,
    READING = 8,
    WRITING = 9,
    REMOTE_CLOSED = 10,
    EVENT_HANDLER = 11,
    NO_ERROR = 12
} ConnectionCloseCause;

typedef enum e_ConnectionOwner {
    READER = 0,
    WORKER = 1,
    WRITER = 2,
    ACCEPTOR = 3,
    AIO4C_MAX_OWNERS = 4
} ConnectionOwner;

#ifndef __AIO4C_CONNECTION_DEFINED__
#define __AIO4C_CONNECTION_DEFINED__

typedef struct s_Connection Connection;

#endif

struct s_Connection {
    Buffer*              readBuffer;
    Buffer*              writeBuffer;
    Buffer*              dataBuffer;
    aio4c_socket_t       socket;
    Address*             address;
    ConnectionState      state;
    EventQueue*          systemHandlers;
    EventQueue*          userHandlers;
    Lock*                lock;
    ConnectionCloseCause closeCause;
    int                  closeCode;
    char*                string;
    aio4c_bool_t         closedBy[AIO4C_MAX_OWNERS];
    SelectionKey*        readKey;
    SelectionKey*        writeKey;
    BufferPool*          pool;
};

#define aio4c_connection_handler(handler) \
    (void(*)(Event,Connection*,void*))handler

#define aio4c_connection_handler_arg(arg) \
    (void*)arg

extern Connection* NewConnection(Buffer* readBuffer, Buffer* writeBuffer, Address* address);

extern Connection* NewConnectionFactory(BufferPool* pool);

extern Connection* ConnectionFactoryCreate(Connection* factory, Address* address, aio4c_socket_t sock);

extern Connection* ConnectionInit(Connection* connection);

extern Connection* ConnectionConnect(Connection* connection);

extern Connection* ConnectionFinishConnect(Connection* connection);

extern Connection* ConnectionRead(Connection* connection);

extern Connection* ConnectionWrite(Connection* connection);

extern Connection* ConnectionClose(Connection* connection);

extern aio4c_bool_t ConnectionNoMoreUsed(Connection* connection, ConnectionOwner owner);

extern Connection* ConnectionAddHandler(Connection* connection, Event event, void (*handler)(Event,Connection*,void*), void* arg, aio4c_bool_t once);

extern Connection* ConnectionAddSystemHandler(Connection* connection, Event event, void (*handler)(Event,Connection*,void*), void* arg, aio4c_bool_t once);

extern Connection* ConnectionEventHandle(Connection* connection, Event event, void* source);

extern void FreeConnection(Connection** connection);

#endif
