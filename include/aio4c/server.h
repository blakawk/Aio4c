/**
 * This file is part of Aio4c <http://aio4c.so>.
 *
 * Aio4c <http://aio4c.so> is free software: you
 * can  redistribute  it  and/or modify it under
 * the  terms  of the GNU General Public License
 * as published by the Free Software Foundation,
 * version 3 of the License.
 *
 * This  program is distributed in the hope that
 * it  will be useful, but WITHOUT ANY WARRANTY;
 * without   even   the   implied   warranty  of
 * MERCHANTABILITY  or  FITNESS FOR A PARTICULAR
 * PURPOSE.
 *
 * See  the  GNU General Public License for more
 * details.  You  should have received a copy of
 * the  GNU  General  Public  License along with
 * Aio4c    <http://aio4c.so>.   If   not,   see
 * <http://www.gnu.org/licenses/>.
 */
#ifndef __AIO4C_SERVER_H__
#define __AIO4C_SERVER_H__

#include <aio4c/acceptor.h>
#include <aio4c/address.h>
#include <aio4c/buffer.h>
#include <aio4c/connection.h>
#include <aio4c/log.h>
#include <aio4c/thread.h>
#include <aio4c/types.h>

#define aio4c_server_handler(handler) \
    (void(*)(Event,Connection*,void*))handler

#define aio4c_server_handler_arg(arg) \
    (void*)arg

#define aio4c_server_factory(factory) \
    (void*(*)(Connection*,void*))factory

typedef struct s_Server {
    Address*    address;
    Acceptor*   acceptor;
    BufferPool* pool;
    Connection* factory;
    Thread*     thread;
    int         nbPipes;
    void      (*handler)(Event,Connection*,void*);
    Queue*      queue;
} Server;

extern AIO4C_API Server* NewServer(AddressType type, char* host, aio4c_port_t port, int bufferSize, int nbPipes, void (*handler)(Event,Connection*,void*), void* handlerArg, void* (*dataFactory)(Connection*,void*));

extern AIO4C_API void ServerJoin(Server* server);

extern AIO4C_API void ServerStop(Server* server);

#endif
