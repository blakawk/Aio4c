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
#include <aio4c/error.h>
#include <aio4c/log.h>
#include <aio4c/types.h>

#include <string.h>

#ifndef AIO4C_WIN32

#include <errno.h>

#endif /* AIO4C_WIN32 */

static aio4c_bool_t _serverInit(Server* server) {
    ConnectionAddHandler(server->factory, AIO4C_CONNECTED_EVENT, aio4c_connection_handler(server->handler), NULL, true);
    ConnectionAddHandler(server->factory, AIO4C_INBOUND_DATA_EVENT, aio4c_connection_handler(server->handler), NULL, false);
    ConnectionAddHandler(server->factory, AIO4C_WRITE_EVENT, aio4c_connection_handler(server->handler), NULL, false);
    ConnectionAddHandler(server->factory, AIO4C_CLOSE_EVENT, aio4c_connection_handler(server->handler), NULL, true);
    server->acceptor = NewAcceptor(server->address, server->factory, server->nbPipes);

    if (server->acceptor == NULL) {
        return false;
    }

    Log(AIO4C_LOG_LEVEL_DEBUG, "initialized with tid 0x%08lx", server->thread->id);

    return true;
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

    FreeAddress(&server->address);
    FreeBufferPool(&server->pool);
    FreeConnection(&server->factory);
    FreeQueue(&server->queue);
}

Server* NewServer(AddressType type, char* host, aio4c_port_t port, int bufferSize, int nbPipes, void (*handler)(Event,Connection*,void*), void* (*dataFactory)(Connection*)) {
    Server* server = NULL;
    ErrorCode code = AIO4C_ERROR_CODE_INITIALIZER;

    if ((server = aio4c_malloc(sizeof(Server))) == NULL) {
#ifndef AIO4C_WIN32
        code.error = errno;
#else /* AIO4C_WIN32 */
        code.source = AIO4C_ERRNO_SOURCE_SYS;
#endif /* AIO4C_WIN32 */
        code.size = sizeof(Server);
        code.type = "Server";
        Raise(AIO4C_LOG_LEVEL_ERROR, AIO4C_ALLOC_ERROR_TYPE, AIO4C_ALLOC_ERROR, &code);
        return NULL;
    }

    server->address    = NewAddress(type, host, port);
    server->pool       = NewBufferPool(bufferSize);
    server->factory    = NewConnectionFactory(server->pool, dataFactory);
    server->acceptor   = NULL;
    server->thread     = NULL;
    server->handler    = handler;
    server->queue      = NewQueue();
    server->nbPipes    = nbPipes;
    server->thread     = NULL;
    NewThread("server",
            aio4c_thread_init(_serverInit),
            aio4c_thread_run(_serverRun),
            aio4c_thread_exit(_serverExit),
            aio4c_thread_arg(server),
            &server->thread);
    if (server->thread == NULL) {
        FreeAddress(&server->address);
        FreeBufferPool(&server->pool);
        FreeConnection(&server->factory);
        FreeQueue(&server->queue);
        aio4c_free(server);
        return NULL;
    }

    return server;
}

void ServerEnd(Server* server) {
    if (server->thread != NULL) {
        EnqueueExitItem(server->queue);
        ThreadJoin(server->thread);
    }
    aio4c_free(server);
}
