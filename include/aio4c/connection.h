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
    CLOSED = 4
} ConnectionState;

#define MAX_PRIORITY 2

typedef enum e_ConnectionEventPriority {
    LOW_PRIORITY = 0,
    HIGH_PRIORITY = 1
} ConnectionEventPriority;


typedef enum e_ConnectionCloseCauseType {
    UNKNOWN,
    INVALID_STATE,
    SOCKET_CREATION,
    SOCKET_NONBLOCK,
    GETSOCKOPT,
    CONNECTING_ERROR,
    BUFFER_UNDERFLOW,
    BUFFER_OVERFLOW,
    READING,
    WRITING,
    REMOTE_CLOSED,
    EVENT_HANDLER,
    NO_ERROR
} ConnectionCloseCause;

typedef struct s_Connection {
    Buffer*              readBuffer;
    Buffer*              writeBuffer;
    aio4c_socket_t       socket;
    Address*             address;
    ConnectionState      state;
    EventQueue*          systemHandlers;
    EventQueue*          userHandlers;
    Lock*                lock;
    ConnectionCloseCause closeCause;
    int                  closeCode;
    char*                string;
} Connection;

#define aio4c_connection_handler(handler) \
    (void(*)(Event,Connection*,void*))handler

#define aio4c_connection_handler_arg(arg) \
    (void*)arg

extern Connection* NewConnection(aio4c_size_t bufferSize, Address* address);

extern Connection* ConnectionInit(Connection* connection);

extern Connection* ConnectionConnect(Connection* connection);

extern Connection* ConnectionFinishConnect(Connection* connection);

extern Connection* ConnectionRead(Connection* connection);

extern Connection* ConnectionWrite(Connection* connection, Buffer* buffer);

extern Connection* ConnectionClose(Connection* connection);

extern Connection* ConnectionAddHandler(Connection* connection, Event event, void (*handler)(Event,Connection*,void*), void* arg, aio4c_bool_t once);

extern Connection* ConnectionAddSystemHandler(Connection* connection, Event event, void (*handler)(Event,Connection*,void*), void* arg, aio4c_bool_t once);

extern Connection* ConnectionEventHandle(Connection* connection, Event event, void* source);

extern void FreeConnection(Connection** connection);

#endif
