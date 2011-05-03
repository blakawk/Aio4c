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
#include <aio4c/acceptor.h>

#include <aio4c/address.h>
#include <aio4c/alloc.h>
#include <aio4c/connection.h>
#include <aio4c/error.h>
#include <aio4c/lock.h>
#include <aio4c/log.h>
#include <aio4c/queue.h>
#include <aio4c/reader.h>
#include <aio4c/selector.h>
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

#include <limits.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>

struct s_Acceptor {
    char*          name;
    Thread*        thread;
    Address*       address;
    aio4c_socket_t socket;
    Reader**       readers;
    int            nbReaders;
    Selector*      selector;
    SelectionKey*  key;
    Connection*    factory;
    Queue*         queue;
};

static aio4c_bool_t _AcceptorInit(Acceptor* acceptor) {
    int i = 0;
    aio4c_addr_t* addr = NULL;
    int addrSize = 0;
#ifndef AIO4C_WIN32
    int reuseaddr = 1;
#else /* AIO4C_WIN32 */
    char reuseaddr = 1;
#endif /* AIO4C_WIN32 */
    ErrorCode code = AIO4C_ERROR_CODE_INITIALIZER;
    char* pipeName = NULL;

    for (i = 0; i < acceptor->nbReaders; i++) {
        pipeName = aio4c_malloc(8);

        if (pipeName != NULL) {
            snprintf(pipeName, 8, "pipe%03d", i);
        }

        if ((acceptor->readers[i] = NewReader(pipeName, GetBufferPoolBufferSize(acceptor->factory->pool))) == NULL) {
            break;
        }
    }

    if (i < acceptor->nbReaders) {
        Log(AIO4C_LOG_LEVEL_WARN, "only %d pipes over %d were started", i, acceptor->nbReaders);
        acceptor->nbReaders = i;
    }

    acceptor->socket = socket(PF_INET, SOCK_STREAM, 0);
#ifndef AIO4C_WIN32
    if (acceptor->socket == -1) {
        code.error = errno;
#else /* AIO4C_WIN32 */
    if (acceptor->socket == SOCKET_ERROR) {
        code.source = AIO4C_ERRNO_SOURCE_WSA;
        acceptor->socket = -1;
#endif /* AIO4C_WIN32 */
        Raise(AIO4C_LOG_LEVEL_ERROR, AIO4C_SOCKET_ERROR_TYPE, AIO4C_SOCKET_ERROR, &code);
        return false;
    }

#ifndef AIO4C_WIN32
    if (fcntl(acceptor->socket, F_SETFL, O_NONBLOCK) == -1) {
        code.error = errno;
#else /* AIO4C_WIN32 */
    unsigned long ioctl = 1;
    if (ioctlsocket(acceptor->socket, FIONBIO, &ioctl) == SOCKET_ERROR) {
        code.source = AIO4C_ERRNO_SOURCE_WSA;
#endif /* AIO4C_WIN32 */
        Raise(AIO4C_LOG_LEVEL_ERROR, AIO4C_SOCKET_ERROR_TYPE, AIO4C_FCNTL_ERROR, &code);
#ifndef AIO4C_WIN32
        close(acceptor->socket);
#else /* AIO4C_WIN32 */
        closesocket(acceptor->socket);
#endif /* AIO4C_WIN32 */
        acceptor->socket = -1;
        return false;
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
        closesocket(acceptor->socket);
#endif /* AIO4C_WIN32 */
        acceptor->socket = -1;
        return false;
    }

    addr = AddressGetAddr(acceptor->address);
    addrSize = AddressGetAddrSize(acceptor->address);

    if (bind(acceptor->socket, addr, addrSize) == -1) {
#ifndef AIO4C_WIN32
        close(acceptor->socket);
#else /* AIO4C_WIN32 */
        closesocket(acceptor->socket);
#endif /* AIO4C_WIN32 */
        acceptor->socket = -1;
        return false;
    }

    if (listen(acceptor->socket, SOMAXCONN) == -1) {
#ifndef AIO4C_WIN32
        close(acceptor->socket);
#else /* AIO4C_WIN32 */
        closesocket(acceptor->socket);
#endif /* AIO4C_WIN32 */
        return false;
    }

    acceptor->key = Register(acceptor->selector, AIO4C_OP_READ, acceptor->socket, NULL);

    Log(AIO4C_LOG_LEVEL_DEBUG, "using %d pipes to manage incoming connections", acceptor->nbReaders);
    Log(AIO4C_LOG_LEVEL_INFO, "listening on %s", AddressGetString(acceptor->address));
    return true;
}

static Reader* _ChooseReader(Acceptor* acceptor) {
    int i = 0, minLoad = INT_MAX, choosen = -1;

    for (i = 0; i < acceptor->nbReaders; i++) {
        if (acceptor->readers[i]->load < minLoad) {
            choosen = i;
            minLoad = acceptor->readers[i]->load;
        }
    }

    return acceptor->readers[choosen];
}

static aio4c_bool_t _AcceptorRun(Acceptor* acceptor) {
    struct sockaddr addr;
    struct sockaddr_in addr_in;
    struct sockaddr_in6 addr_in6;
#ifndef AIO4C_WIN32
    struct sockaddr_un addr_un;
#endif /* AIO4C_WIN32 */
    socklen_t addrSize = sizeof(struct sockaddr);
    aio4c_size_t numConnectionsReady = 0;
    aio4c_socket_t sock = -1;
    Address* address = NULL;
    Connection* connection = NULL;
    char hbuf[NI_MAXHOST];
    SelectionKey* key = NULL;

    memset(&addr, 0, sizeof(struct sockaddr));
    memset(&addr_in, 0, sizeof(struct sockaddr_in));
    memset(&addr_in6, 0, sizeof(struct sockaddr_in6));
#ifndef AIO4C_WIN32
    memset(&addr_un, 0, sizeof(struct sockaddr_un));
#endif /* AIO4C_WIN32 */

    numConnectionsReady = Select(acceptor->selector);

    if (numConnectionsReady > 0) {
        while (SelectionKeyReady(acceptor->selector, &key)) {
            if ((sock = accept(acceptor->socket, &addr, &addrSize)) >= 0) {
                memset(hbuf, 0, sizeof(hbuf));

                switch(AddressGetType(acceptor->address)) {
                    case AIO4C_ADDRESS_IPV4:
                        if (getnameinfo(&addr, sizeof(struct sockaddr_in), hbuf, sizeof(hbuf), NULL, 0, NI_NUMERICHOST) == 0) {
                            memcpy(&addr_in, &addr, sizeof(struct sockaddr_in));    
                            address = NewAddress(AIO4C_ADDRESS_IPV4, hbuf, ntohs(addr_in.sin_port));
                        }
                        break;
                    case AIO4C_ADDRESS_IPV6:
                        if (getnameinfo(&addr, sizeof(struct sockaddr_in6), hbuf, sizeof(hbuf), NULL, 0, NI_NUMERICHOST) == 0) {
                            memcpy(&addr_in6, &addr, sizeof(struct sockaddr_in6));
                            address = NewAddress(AIO4C_ADDRESS_IPV6, hbuf, ntohs(addr_in6.sin6_port));
                        }
                        break;
#ifndef AIO4C_WIN32
                    case AIO4C_ADDRESS_UNIX:
                        memcpy(&addr_un, &addr, sizeof(struct sockaddr_un));
                        address = NewAddress(AIO4C_ADDRESS_UNIX, addr_un.sun_path, 0);
                        break;
#endif /* AIO4C_WIN32 */
                    default:
                        break;
                }

                Log(AIO4C_LOG_LEVEL_INFO, "new connection from %s", AddressGetString(address));

                connection = ConnectionFactoryCreate(acceptor->factory, address, sock);

                EnqueueDataItem(acceptor->queue, connection);

                ReaderManageConnection(_ChooseReader(acceptor), connection);

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

    Log(AIO4C_LOG_LEVEL_DEBUG, "received close for connection %s", source->string);

    RemoveAll(acceptor->queue, aio4c_remove_callback(_AcceptorRemoveCallback), aio4c_remove_discriminant(source));

    if (ConnectionNoMoreUsed(source, AIO4C_CONNECTION_OWNER_ACCEPTOR)) {
        FreeConnection(&source);
    }

    ProbeSize(AIO4C_PROBE_CONNECTION_COUNT, -1);
}

static void _AcceptorExit(Acceptor* acceptor) {
    QueueItem item;
    Connection* connection = NULL;
    int i = 0;

    memset(&item, 0, sizeof(QueueItem));

    if (acceptor->key != NULL) {
        Unregister(acceptor->selector, acceptor->key, true, NULL);
    }

    if (acceptor->socket != -1) {
#ifndef AIO4C_WIN32
        close(acceptor->socket);
#else /* AIO4C_WIN32 */
        closesocket(acceptor->socket);
#endif /* AIO4C_WIN32 */
    }

    while (acceptor->queue != NULL && Dequeue(acceptor->queue, &item, false)) {
        connection = (Connection*)item.content.data;
        EventHandlerRemove(connection->systemHandlers, AIO4C_CLOSE_EVENT, aio4c_event_handler(_AcceptorCloseHandler));
        ConnectionClose(connection, true);
        if (ConnectionNoMoreUsed(connection, AIO4C_CONNECTION_OWNER_ACCEPTOR)) {
            FreeConnection(&connection);
        }
    }

    FreeQueue(&acceptor->queue);

    if (acceptor->readers != NULL) {
        for (i = 0; i < acceptor->nbReaders; i++) {
            if (acceptor->readers[i] != NULL) {
                ReaderEnd(acceptor->readers[i]);
            }
        }
        aio4c_free(acceptor->readers);
    }

    Log(AIO4C_LOG_LEVEL_DEBUG, "exited");
}

Acceptor* NewAcceptor(char* name, Address* address, Connection* factory, int nbPipes) {
    Acceptor* acceptor = NULL;
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

    acceptor->name = name;
    acceptor->address = address;
    acceptor->factory = factory;
    acceptor->queue = NewQueue();
    ConnectionAddSystemHandler(acceptor->factory, AIO4C_CLOSE_EVENT, aio4c_connection_handler(_AcceptorCloseHandler), aio4c_connection_handler_arg(acceptor), true);
    acceptor->socket = -1;
    acceptor->nbReaders = nbPipes;
    if ((acceptor->readers = aio4c_malloc(nbPipes * sizeof(Reader*))) == NULL) {
#ifndef AIO4C_WIN32
        code.error = errno;
#else /* AIO4C_WIN32 */
        code.source = AIO4C_ERRNO_SOURCE_SYS;
#endif /* AIO4C_WIN32 */
        code.size = nbPipes * sizeof(Reader*);
        code.type = "Reader*";
        Raise(AIO4C_LOG_LEVEL_ERROR, AIO4C_ALLOC_ERROR_TYPE, AIO4C_ALLOC_ERROR, &code);
        FreeAddress(&acceptor->address);
        aio4c_free(acceptor);
        return NULL;
    }

    acceptor->thread = NULL;
    acceptor->selector = NewSelector();

    acceptor->thread = NewThread(
            acceptor->name,
            aio4c_thread_init(_AcceptorInit),
            aio4c_thread_run(_AcceptorRun),
            aio4c_thread_exit(_AcceptorExit),
            aio4c_thread_arg(acceptor));

    if (acceptor->thread == NULL) {
        FreeAddress(&acceptor->address);
        FreeSelector(&acceptor->selector);
        if (acceptor->name != NULL) {
            aio4c_free(acceptor->name);
        }
        aio4c_free(acceptor);
        return NULL;
    }

    if (!ThreadStart(acceptor->thread)) {
        FreeAddress(&acceptor->address);
        FreeSelector(&acceptor->selector);
        if (acceptor->name != NULL) {
            aio4c_free(acceptor->name);
        }
        aio4c_free(acceptor);
        return NULL;
    }

    return acceptor;
}

void AcceptorEnd(Acceptor* acceptor) {
    if (acceptor->thread != NULL) {
        acceptor->thread->running = false;

        if (acceptor->selector != NULL) {
            SelectorWakeUp(acceptor->selector);
        }

        ThreadJoin(acceptor->thread);
    }

    FreeSelector(&acceptor->selector);

    aio4c_free(acceptor);
}

