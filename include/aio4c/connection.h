/**
 * Copyright (c) 2011 blakawk
 *
 * This file is part of Aio4c <http://aio4c.so>.
 *
 * Aio4c <http://aio4c.so> is free software: you
 * can  redistribute  it  and/or modify it under
 * the  terms  of the GNU General Public License
 * as published by the Free Software Foundation,
 * version 3 of the License.
 *
 * Aio4c <http://aio4c.so> is distributed in the
 * hope  that it will be useful, but WITHOUT ANY
 * WARRANTY;  without  even the implied warranty
 * of   MERCHANTABILITY   or   FITNESS   FOR   A
 * PARTICULAR PURPOSE.
 *
 * See  the  GNU General Public License for more
 * details.  You  should have received a copy of
 * the  GNU  General  Public  License along with
 * Aio4c    <http://aio4c.so>.   If   not,   see
 * <http://www.gnu.org/licenses/>.
 */
#ifndef __AIO4C_CONNECTION_H__
#define __AIO4C_CONNECTION_H__

#include <aio4c/address.h>
#include <aio4c/buffer.h>
#include <aio4c/event.h>
#include <aio4c/lock.h>
#include <aio4c/selector.h>
#include <aio4c/types.h>

typedef enum e_ConnectionState {
    AIO4C_CONNECTION_STATE_NONE,
    AIO4C_CONNECTION_STATE_INITIALIZED,
    AIO4C_CONNECTION_STATE_CONNECTING,
    AIO4C_CONNECTION_STATE_CONNECTED,
    AIO4C_CONNECTION_STATE_PENDING_CLOSE,
    AIO4C_CONNECTION_STATE_CLOSED,
    AIO4C_CONNECTION_STATE_MAX
} ConnectionState;

extern char* ConnectionStateString[AIO4C_CONNECTION_STATE_MAX];

typedef enum e_ConnectionOwner {
    AIO4C_CONNECTION_OWNER_READER,
    AIO4C_CONNECTION_OWNER_WORKER,
    AIO4C_CONNECTION_OWNER_WRITER,
    AIO4C_CONNECTION_OWNER_ACCEPTOR,
    AIO4C_CONNECTION_OWNER_CLIENT,
    AIO4C_CONNECTION_OWNER_MAX
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
    Lock*                stateLock;
    EventQueue*          systemHandlers;
    EventQueue*          userHandlers;
    bool         closedForError;
    char*                string;
    bool         closedBy[AIO4C_CONNECTION_OWNER_MAX];
    Lock*                closedByLock;
    bool         managedBy[AIO4C_CONNECTION_OWNER_MAX];
    Lock*                managedByLock;
    SelectionKey*        readKey;
    SelectionKey*        writeKey;
    BufferPool*          pool;
    bool         freeAddress;
    bool         canRead;
    bool         canWrite;
    void*              (*dataFactory)(Connection*,void*);
    void*                dataFactoryArg;
    bool         isFactory;
};

#define aio4c_connection_handler(handler) \
    (void(*)(Event,Connection*,void*))handler

#define aio4c_connection_handler_arg(arg) \
    (void*)arg

extern AIO4C_API Connection* NewConnection(BufferPool* pool, Address* address, bool freeAddress);

extern AIO4C_API Connection* NewConnectionFactory(BufferPool* pool, void* (*dataFactory)(Connection*,void*), void* dataFactoryArg);

extern AIO4C_API Connection* ConnectionFactoryCreate(Connection* factory, Address* address, aio4c_socket_t sock);

#define ConnectionState(connection,state) \
    _ConnectionState(__FILE__, __LINE__, connection, state)
extern AIO4C_API void _ConnectionState(char* file, int line, Connection* connection, ConnectionState state);

extern AIO4C_API Connection* ConnectionInit(Connection* connection);

extern AIO4C_API Connection* ConnectionConnect(Connection* connection);

extern AIO4C_API Connection* ConnectionFinishConnect(Connection* connection);

extern AIO4C_API Connection* ConnectionRead(Connection* connection);

extern AIO4C_API Connection* ConnectionProcessData(Connection* connection);

extern AIO4C_API void EnableWriteInterest(Connection* connection);

extern AIO4C_API bool ConnectionWrite(Connection* connection);

extern AIO4C_API Connection* ConnectionShutdown(Connection* connection);

extern AIO4C_API Connection* ConnectionClose(Connection* connection, bool force);

extern AIO4C_API bool ConnectionNoMoreUsed(Connection* connection, ConnectionOwner owner);

extern AIO4C_API void ConnectionManagedBy(Connection* connection, ConnectionOwner owner);

extern AIO4C_API Connection* ConnectionAddHandler(Connection* connection, Event event, void (*handler)(Event,Connection*,void*), void* arg, bool once);

extern AIO4C_API Connection* ConnectionAddSystemHandler(Connection* connection, Event event, void (*handler)(Event,Connection*,void*), void* arg, bool once);

extern AIO4C_API Buffer* ConnectionGetWriteBuffer(Connection* connection);

extern AIO4C_API Buffer* ConnectionGetReadBuffer(Connection* connection);

extern AIO4C_API char* ConnectionGetString(Connection* connection);

extern AIO4C_API void FreeConnection(Connection** connection);

#endif
