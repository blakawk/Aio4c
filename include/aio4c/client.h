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
#ifndef __AIO4C_CLIENT_H__
#define __AIO4C_CLIENT_H__

#include <aio4c/address.h>
#include <aio4c/log.h>
#include <aio4c/reader.h>
#include <aio4c/thread.h>
#include <aio4c/types.h>

typedef struct s_Client {
    Address*     address;
    Connection*  connection;
    Thread*      thread;
    Reader*      reader;
    Queue*       queue;
    BufferPool*  pool;
    void       (*handler)(Event,Connection*,void*);
    void*        handlerArg;
    int          retries;
    int          interval;
    int          retryCount;
    int          bufferSize;
    aio4c_bool_t connected;
    Thread*      main;
} Client;

#define aio4c_client_handler(h) \
    (void(*)(Event,Connection*,void*))h

#define aio4c_client_handler_arg(h) \
    (void*)h

extern __aio4c_dll Client* NewClient(AddressType type, char* address, aio4c_port_t port, LogLevel level, char* log, int retries, int retryInterval, int bufferSize, void (*handler)(Event,Connection*,void*), void *handlerArg);

extern __aio4c_dll void ClientEnd(Client* client);

#endif
