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
#include <aio4c/lock.h>
#include <aio4c/selector.h>
#include <aio4c/types.h>

typedef enum e_ConnectionState {
    AIO4C_CONNECTION_STATE_NONE = 0,
    AIO4C_CONNECTION_STATE_INITIALIZED = 1,
    AIO4C_CONNECTION_STATE_CONNECTING = 2,
    AIO4C_CONNECTION_STATE_CONNECTED = 3,
    AIO4C_CONNECTION_STATE_CLOSED = 4,
    AIO4C_CONNECTION_STATE_MAX = 5
} ConnectionState;

extern char* ConnectionStateString[AIO4C_CONNECTION_STATE_MAX];

typedef enum e_ConnectionCloseCauseType {
    AIO4C_CONNECTION_CLOSE_CAUSE_UNKNOWN = 0,
    AIO4C_CONNECTION_CLOSE_CAUSE_INVALID_STATE = 1,
    AIO4C_CONNECTION_CLOSE_CAUSE_SOCKET_CREATION = 2,
    AIO4C_CONNECTION_CLOSE_CAUSE_SOCKET_NONBLOCK = 3,
    AIO4C_CONNECTION_CLOSE_CAUSE_GETSOCKOPT = 4,
    AIO4C_CONNECTION_CLOSE_CAUSE_CONNECTING_ERROR = 5,
    AIO4C_CONNECTION_CLOSE_CAUSE_BUFFER_UNDERFLOW = 6,
    AIO4C_CONNECTION_CLOSE_CAUSE_BUFFER_OVERFLOW = 7,
    AIO4C_CONNECTION_CLOSE_CAUSE_READING = 8,
    AIO4C_CONNECTION_CLOSE_CAUSE_WRITING = 9,
    AIO4C_CONNECTION_CLOSE_CAUSE_REMOTE_CLOSED = 10,
    AIO4C_CONNECTION_CLOSE_CAUSE_EVENT_HANDLER = 11,
    AIO4C_CONNECTION_CLOSE_CAUSE_NO_ERROR = 12,
    AIO4C_CONNECTION_CLOSE_CAUSE_MAX = 13
} ConnectionCloseCause;

typedef enum e_ConnectionOwner {
    AIO4C_CONNECTION_OWNER_READER = 0,
    AIO4C_CONNECTION_OWNER_WORKER = 1,
    AIO4C_CONNECTION_OWNER_WRITER = 2,
    AIO4C_CONNECTION_OWNER_ACCEPTOR = 3,
    AIO4C_CONNECTION_OWNER_MAX = 4
} ConnectionOwner;

#ifndef __AIO4C_CONNECTION_DEFINED__
#define __AIO4C_CONNECTION_DEFINED__
typedef struct s_Connection Connection;
#endif /* __AIO4C_CONNECTION_DEFINED__ */

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
    aio4c_bool_t         closedBy[AIO4C_CONNECTION_OWNER_MAX];
    SelectionKey*        readKey;
    SelectionKey*        writeKey;
    BufferPool*          pool;
    aio4c_bool_t         freeAddress;
    void*              (*dataFactory)(Connection*);
};

#define aio4c_connection_handler(handler) \
    (void(*)(Event,Connection*,void*))handler

#define aio4c_connection_handler_arg(arg) \
    (void*)arg

extern Connection* NewConnection(BufferPool* pool, Address* address, aio4c_bool_t freeAddress);

extern Connection* NewConnectionFactory(BufferPool* pool, void* (*dataFactory)(Connection*));

extern Connection* ConnectionFactoryCreate(Connection* factory, Address* address, aio4c_socket_t sock);

extern Connection* ConnectionInit(Connection* connection);

extern Connection* ConnectionConnect(Connection* connection);

extern Connection* ConnectionFinishConnect(Connection* connection);

extern Connection* ConnectionRead(Connection* connection);

extern AIO4C_DLLEXPORT void EnableWriteInterest(Connection* connection);

extern Connection* ConnectionWrite(Connection* connection);

extern AIO4C_DLLEXPORT Connection* ConnectionClose(Connection* connection);

extern aio4c_bool_t ConnectionNoMoreUsed(Connection* connection, ConnectionOwner owner);

extern Connection* ConnectionAddHandler(Connection* connection, Event event, void (*handler)(Event,Connection*,void*), void* arg, aio4c_bool_t once);

extern Connection* ConnectionAddSystemHandler(Connection* connection, Event event, void (*handler)(Event,Connection*,void*), void* arg, aio4c_bool_t once);

extern Connection* ConnectionEventHandle(Connection* connection, Event event, void* source);

extern void FreeConnection(Connection** connection);

#endif
