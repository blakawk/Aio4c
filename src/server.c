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
#include <aio4c/server.h>

#include <aio4c/acceptor.h>
#include <aio4c/address.h>
#include <aio4c/alloc.h>
#include <aio4c/buffer.h>
#include <aio4c/log.h>
#include <aio4c/types.h>

#include <string.h>

static void _serverInit(Server* server) {
    ConnectionAddHandler(server->factory, AIO4C_CONNECTED_EVENT, aio4c_connection_handler(server->handler), aio4c_connection_handler_arg(server->handlerArg), true);
    ConnectionAddHandler(server->factory, AIO4C_INBOUND_DATA_EVENT, aio4c_connection_handler(server->handler), aio4c_connection_handler_arg(server->handlerArg), false);
    ConnectionAddHandler(server->factory, AIO4C_WRITE_EVENT, aio4c_connection_handler(server->handler), aio4c_connection_handler_arg(server->handlerArg), false);
    ConnectionAddHandler(server->factory, AIO4C_CLOSE_EVENT, aio4c_connection_handler(server->handler), aio4c_connection_handler_arg(server->handlerArg), true);
    server->acceptor = NewAcceptor("server-acceptor", server->address, server->factory);
    Log(server->thread, AIO4C_LOG_LEVEL_INFO, "listening on %s", server->address->string);
}

static aio4c_bool_t _serverRun(Server* server) {
    QueueItem item;

    memset(&item, 0, sizeof(QueueItem));

    while (Dequeue(server->queue, &item, true)) {
        if (item.type == AIO4C_QUEUE_ITEM_EXIT) {
            return false;
        }
    }

    return true;
}

static void _serverExit(Server* server) {
    if (server->acceptor != NULL) {
        AcceptorEnd(server->acceptor);
    }

    LogEnd();

    FreeAddress(&server->address);
    FreeBufferPool(&server->pool);
    FreeConnection(&server->factory);
    FreeQueue(&server->queue);
    FreeThread(&server->main);
    aio4c_free(server);
}

Server* NewServer(AddressType type, char* host, aio4c_port_t port, LogLevel level, char* log, int bufferSize, void (*handler)(Event,Connection*,void*), void* handlerArg) {
    Server* server = NULL;

    if ((server = aio4c_malloc(sizeof(Server))) == NULL) {
        return NULL;
    }

    server->main       = ThreadMain("aio4c-main");
    LogInit(server->main, level, log);

    server->address    = NewAddress(type, host, port);
    server->pool       = NewBufferPool(2, bufferSize);
    server->factory    = NewConnectionFactory(server->pool);
    server->acceptor   = NULL;
    server->thread     = NULL;
    server->handler    = handler;
    server->handlerArg = handlerArg;
    server->queue      = NewQueue();
    server->thread     = NewThread("server",
            aio4c_thread_handler(_serverInit),
            aio4c_thread_run(_serverRun),
            aio4c_thread_handler(_serverExit),
            aio4c_thread_arg(server));

    return server;
}

void ServerEnd(Server* server) {
    Thread* sThread = server->thread;
    EnqueueExitItem(server->queue);
    ThreadJoin(sThread);
    FreeThread(&sThread);
}
