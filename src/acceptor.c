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
#include <aio4c/error.h>
#include <aio4c/log.h>
#include <aio4c/reader.h>
#include <aio4c/stats.h>
#include <aio4c/thread.h>
#include <aio4c/types.h>

#ifndef AIO4C_WIN32

#include <arpa/inet.h>
#include <errno.h>
#include <fcntl.h>
#include <netdb.h>
#include <sys/socket.h>
#include <sys/un.h>
#include <unistd.h>

#else /* AIO4C_WIN32 */

#include <winsock2.h>
#include <ws2tcpip.h>

#endif /* AIO4C_WIN32 */

#include <stdlib.h>
#include <string.h>
#include <sys/types.h>

static void _AcceptorInit(Acceptor* acceptor) {
    acceptor->reader = NewReader(acceptor->factory->pool->bufferSize);
    acceptor->key = Register(acceptor->selector, AIO4C_OP_READ, acceptor->socket, NULL);
    Log(acceptor->thread, AIO4C_LOG_LEVEL_INFO, "initialized");
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
                    case AIO4C_ADDRESS_IPV4:
                        if ((host = aio4c_malloc(INET_ADDRSTRLEN * sizeof(char))) != NULL) {
                            if (getnameinfo((struct sockaddr*)&addr, sizeof(struct sockaddr_in), host, INET_ADDRSTRLEN, NULL, 0, NI_NUMERICHOST) == 0) {
                                address = NewAddress(AIO4C_ADDRESS_IPV4, host, ntohs(((struct sockaddr_in*)&addr)->sin_port));
                            }
                            aio4c_free(host);
                        }
                        break;
                    case AIO4C_ADDRESS_IPV6:
                        if ((host = aio4c_malloc(INET6_ADDRSTRLEN * sizeof(char))) != NULL) {
                            if (getnameinfo((struct sockaddr*)&addr, sizeof(struct sockaddr_in6), host, INET6_ADDRSTRLEN, NULL, 0, NI_NUMERICHOST) == 0) {
                                address = NewAddress(AIO4C_ADDRESS_IPV6, host, ntohs(((struct sockaddr_in6*)&addr)->sin6_port));
                            }
                            aio4c_free(host);
                        }
                        break;
#ifndef AIO4C_WIN32
                    case AIO4C_ADDRESS_UNIX:
                        address = NewAddress(AIO4C_ADDRESS_UNIX, ((struct sockaddr_un*)&addr)->sun_path, 0);
                        break;
#endif
                    default:
                        break;
                }

                Log(acceptor->thread, AIO4C_LOG_LEVEL_INFO, "new connection from %s", address->string);

                connection = ConnectionFactoryCreate(acceptor->factory, address, sock);

                EnqueueDataItem(acceptor->queue, connection);

                ReaderManageConnection(acceptor->reader, connection);

                ProbeSize(AIO4C_PROBE_CONNECTION_COUNT, 1);
            }
        }
    }

    return true;
}

static aio4c_bool_t _AcceptorRemoveCallback(QueueItem* item, Connection* discriminant) {
    Connection* c = NULL;

    if (item->type == AIO4C_QUEUE_ITEM_DATA) {
        c = (Connection*)item->content.data;
        if (c == discriminant) {
            return true;
        }
    }

    return false;
}

static void _AcceptorCloseHandler(Event event, Connection* source, Acceptor* acceptor) {
    if (event != AIO4C_CLOSE_EVENT) {
        return;
    }

    RemoveAll(acceptor->queue, aio4c_remove_callback(_AcceptorRemoveCallback), aio4c_remove_discriminant(source));

    if (ConnectionNoMoreUsed(source, AIO4C_CONNECTION_OWNER_ACCEPTOR)) {
        FreeConnection(&source);
    }

    ProbeSize(AIO4C_PROBE_CONNECTION_COUNT, -1);
}

static void _AcceptorExit(Acceptor* acceptor) {
    QueueItem item;
    Connection* connection = NULL;

    memset(&item, 0, sizeof(QueueItem));

    Unregister(acceptor->selector, acceptor->key, true);
    close(acceptor->socket);

    while (Dequeue(acceptor->queue, &item, false)) {
        connection = (Connection*)item.content.data;
        EventHandlerRemove(connection->systemHandlers, AIO4C_CLOSE_EVENT, aio4c_event_handler(_AcceptorCloseHandler));
        ConnectionClose(connection);
        if (ConnectionNoMoreUsed(connection, AIO4C_CONNECTION_OWNER_ACCEPTOR)) {
            FreeConnection(&connection);
        }
    }

    FreeQueue(&acceptor->queue);
    ReaderEnd(acceptor->reader);
    FreeSelector(&acceptor->selector);

    Log(acceptor->thread, AIO4C_LOG_LEVEL_INFO, "exited");

    aio4c_free(acceptor);
}

Acceptor* NewAcceptor(Address* address, Connection* factory) {
    Acceptor* acceptor = NULL;
#ifndef AIO4C_WIN32
    int reuseaddr = 1;
#else /* AIO4C_WIN32 */
    char reuseaddr = 1;
#endif /* AIO4C_WIN32 */
    ErrorCode code = AIO4C_ERROR_CODE_INITIALIZER;

    if ((acceptor = aio4c_malloc(sizeof(Acceptor))) == NULL) {
#ifndef AIO4C_WIN32
        code.error = errno;
#else /* AIO4C_WIN32 */
        code.source = AIO4C_ERRNO_SOURCE_SYS;
#endif /* AIO4C_WIN32 */
        code.size = sizeof(Acceptor);
        code.type = "Acceptor";
        Raise(AIO4C_LOG_LEVEL_ERROR, AIO4C_ALLOC_ERROR_TYPE, AIO4C_ALLOC_ERROR, &code);
        return NULL;
    }

    acceptor->address = address;
    acceptor->socket = socket(PF_INET, SOCK_STREAM, 0);
#ifndef AIO4C_WIN32
    if (acceptor->socket == -1) {
        code.error = errno;
#else /* AIO4C_WIN32 */
    if (acceptor->socket == SOCKET_ERROR) {
        code.source = AIO4C_ERRNO_SOURCE_WSA;
#endif /* AIO4C_WIN32 */
        Raise(AIO4C_LOG_LEVEL_ERROR, AIO4C_SOCKET_ERROR_TYPE, AIO4C_SOCKET_ERROR, &code);
        aio4c_free(acceptor);
        return NULL;
    }

    acceptor->factory = factory;
    acceptor->queue = NewQueue();

    ConnectionAddSystemHandler(acceptor->factory, AIO4C_CLOSE_EVENT, aio4c_connection_handler(_AcceptorCloseHandler), aio4c_connection_handler_arg(acceptor), true);

#ifndef AIO4C_WIN32
    if (fcntl(acceptor->socket, F_SETFL, O_NONBLOCK) == -1) {
        code.error = errno;
#else /* AIO4C_WIN32 */
    unsigned long ioctl = 1;
    if (ioctlsocket(acceptor->socket, FIONBIO, &ioctl) == SOCKET_ERROR) {
        code.source = AIO4C_ERRNO_SOURCE_WSA;
#endif /* AIO4C_WIN32 */
        Raise(AIO4C_LOG_LEVEL_ERROR, AIO4C_SOCKET_ERROR_TYPE, AIO4C_FCNTL_ERROR, &code);
        FreeAddress(&acceptor->address);
        aio4c_free(acceptor);
        return NULL;
    }

    if (setsockopt(acceptor->socket, SOL_SOCKET, SO_REUSEADDR, &reuseaddr, sizeof(reuseaddr)) != 0) {
#ifndef AIO4C_WIN32
        code.error = errno;
#else /* AIO4C_WIN32 */
        code.source = AIO4C_ERRNO_SOURCE_WSA;
#endif /* AIO4C_WIN32 */
        Raise(AIO4C_LOG_LEVEL_ERROR, AIO4C_SOCKET_ERROR_TYPE, AIO4C_SETSOCKOPT_ERROR, &code);
#ifndef AIO4C_WIN32
        close(acceptor->socket);
#else /* AIO4C_WIN32 */
        socketclose(acceptor->socket);
#endif /* AIO4C_WIN32 */
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

    acceptor->thread = NewThread("acceptor", aio4c_thread_handler(_AcceptorInit), aio4c_thread_run(_AcceptorRun), aio4c_thread_handler(_AcceptorExit), aio4c_thread_arg(acceptor));

    return acceptor;
}

void AcceptorEnd(Acceptor* acceptor) {
    Thread* th = acceptor->thread;
    acceptor->thread->running = false;
    SelectorWakeUp(acceptor->selector);
    ThreadJoin(th);
    FreeThread(&th);
}

