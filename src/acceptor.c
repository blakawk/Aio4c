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
#include <aio4c/acceptor.h>

#include <aio4c/address.h>
#include <aio4c/alloc.h>
#include <aio4c/connection.h>
#include <aio4c/log.h>
#include <aio4c/reader.h>
#include <aio4c/stats.h>
#include <aio4c/thread.h>
#include <aio4c/types.h>

#include <arpa/inet.h>
#include <fcntl.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <sys/un.h>
#include <unistd.h>

static void _AcceptorInit(Acceptor* acceptor) {
    acceptor->reader = NewReader("reader", acceptor->factory->pool->bufferSize);
    acceptor->key = Register(acceptor->selector, AIO4C_OP_READ, acceptor->socket, NULL);
    Log(acceptor->thread, INFO, "initialized");
}

static aio4c_bool_t _AcceptorRun(Acceptor* acceptor) {
    struct sockaddr addr;
    socklen_t addrSize = sizeof(struct sockaddr);
    aio4c_size_t numConnectionsReady = 0;
    aio4c_socket_t sock = -1;
    Address* address = NULL;
    char* host = NULL;
    Connection* connection = NULL;
    SelectionKey* key = NULL;

    memset(&addr, 0, sizeof(struct sockaddr));

    numConnectionsReady = Select(acceptor->selector);

    if (numConnectionsReady > 0) {
        while (SelectionKeyReady(acceptor->selector, &key)) {
            if ((sock = accept(acceptor->socket, &addr, &addrSize)) >= 0) {
                switch(acceptor->address->type) {
                    case IPV4:
                        if ((host = aio4c_malloc(INET_ADDRSTRLEN * sizeof(char))) != NULL) {
                            if (inet_ntop(AF_INET, &(((struct sockaddr_in*)&addr)->sin_addr), host, INET_ADDRSTRLEN) != NULL) {
                                address = NewAddress(IPV4, host, ntohs(((struct sockaddr_in*)&addr)->sin_port));
                            }
                            aio4c_free(host);
                        }
                        break;
                    case IPV6:
                        if ((host = aio4c_malloc(INET6_ADDRSTRLEN * sizeof(char))) != NULL) {
                            if (inet_ntop(AF_INET6, &(((struct sockaddr_in6*)&addr)->sin6_addr), host, INET6_ADDRSTRLEN) != NULL) {
                                address = NewAddress(IPV6, host, ntohs(((struct sockaddr_in6*)&addr)->sin6_port));
                            }
                            aio4c_free(host);
                        }
                        break;
                    case UNIX:
                        address = NewAddress(UNIX, ((struct sockaddr_un*)&addr)->sun_path, 0);
                        break;
                }

                Log(acceptor->thread, INFO, "new connection from %s", address->string);

                connection = ConnectionFactoryCreate(acceptor->factory, address, sock);

                EnqueueDataItem(acceptor->queue, connection);

                ReaderManageConnection(acceptor->reader, connection);

                ProbeSize(PROBE_CONNECTION_COUNT, 1);
            }
        }
    }

    return true;
}

static aio4c_bool_t _AcceptorRemoveCallback(QueueItem* item, Connection* discriminant) {
    Connection* c = NULL;

    if (item->type == DATA) {
        c = (Connection*)item->content.data;
        if (c == discriminant) {
            return true;
        }
    }

    return false;
}

static void _AcceptorCloseHandler(Event event, Connection* source, Acceptor* acceptor) {
    if (event != CLOSE_EVENT) {
        return;
    }

    RemoveAll(acceptor->queue, aio4c_remove_callback(_AcceptorRemoveCallback), aio4c_remove_discriminant(source));

    if (ConnectionNoMoreUsed(source, ACCEPTOR)) {
        FreeConnection(&source);
    }

    ProbeSize(PROBE_CONNECTION_COUNT, -1);
}

static void _AcceptorExit(Acceptor* acceptor) {
    QueueItem item;
    Connection* connection = NULL;

    memset(&item, 0, sizeof(QueueItem));

    Unregister(acceptor->selector, acceptor->key, true);
    close(acceptor->socket);

    while (Dequeue(acceptor->queue, &item, false)) {
        connection = (Connection*)item.content.data;
        EventHandlerRemove(connection->systemHandlers, CLOSE_EVENT, aio4c_event_handler(_AcceptorCloseHandler));
        ConnectionClose(connection);
        if (ConnectionNoMoreUsed(connection, ACCEPTOR)) {
            FreeConnection(&connection);
        }
    }

    FreeQueue(&acceptor->queue);
    ReaderEnd(acceptor->reader);
    FreeSelector(&acceptor->selector);

    Log(acceptor->thread, INFO, "exited");

    aio4c_free(acceptor);
}

Acceptor* NewAcceptor(char* name, Address* address, Connection* factory) {
    Acceptor* acceptor = NULL;
    int reuseaddr = 1;

    if ((acceptor = aio4c_malloc(sizeof(Acceptor))) == NULL) {
        return NULL;
    }

    acceptor->address = address;
    acceptor->socket = socket(PF_INET, SOCK_STREAM, 0);
    acceptor->factory = factory;
    acceptor->queue = NewQueue();

    ConnectionAddSystemHandler(acceptor->factory, CLOSE_EVENT, aio4c_connection_handler(_AcceptorCloseHandler), aio4c_connection_handler_arg(acceptor), true);

    if (acceptor->socket == -1) {
        FreeAddress(&acceptor->address);
        aio4c_free(acceptor);
        return NULL;
    }

    if (fcntl(acceptor->socket, F_SETFL, O_NONBLOCK) == -1) {
        FreeAddress(&acceptor->address);
        aio4c_free(acceptor);
        return NULL;
    }

    if (setsockopt(acceptor->socket, SOL_SOCKET, SO_REUSEADDR, &reuseaddr, sizeof(reuseaddr)) == -1) {
        FreeAddress(&acceptor->address);
        aio4c_free(acceptor);
        return NULL;
    }

    if (bind(acceptor->socket, (struct sockaddr*)acceptor->address->address, acceptor->address->size) == -1) {
        FreeAddress(&acceptor->address);
        aio4c_free(acceptor);
        return NULL;
    }

    if (listen(acceptor->socket, SOMAXCONN) == -1) {
        FreeAddress(&acceptor->address);
        aio4c_free(acceptor);
        return NULL;
    }

    acceptor->reader = NULL;
    acceptor->thread = NULL;
    acceptor->selector = NewSelector();

    acceptor->thread = NewThread(name, aio4c_thread_handler(_AcceptorInit), aio4c_thread_run(_AcceptorRun), aio4c_thread_handler(_AcceptorExit), aio4c_thread_arg(acceptor));

    return acceptor;
}

void AcceptorEnd(Acceptor* acceptor) {
    Thread* th = acceptor->thread;
    acceptor->thread->state = STOPPED;
    SelectorWakeUp(acceptor->selector);
    ThreadJoin(th);
    FreeThread(&th);
}

