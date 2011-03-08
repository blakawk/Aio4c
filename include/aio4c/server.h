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

typedef struct s_Server {
    Thread*     main;
    Address*    address;
    Acceptor*   acceptor;
    BufferPool* pool;
    Connection* factory;
    Thread*     thread;
    void      (*handler)(Event,Connection*,void*);
    Queue*      queue;
} Server;

extern AIO4C_DLLEXPORT Server* NewServer(AddressType type, char* host, aio4c_port_t port, LogLevel level, char* log, int bufferSize, void (*handler)(Event,Connection*,void*), void* (*dataFactory)(Connection*));

extern AIO4C_DLLEXPORT void ServerEnd(Server* server);

#endif
