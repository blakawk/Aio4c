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
#include <aio4c/client.h>

#include <aio4c/address.h>
#include <aio4c/alloc.h>
#include <aio4c/buffer.h>
#include <aio4c/error.h>
#include <aio4c/log.h>
#include <aio4c/reader.h>
#include <aio4c/stats.h>
#include <aio4c/thread.h>
#include <aio4c/types.h>

#ifdef AIO4C_WIN32
#include <winbase.h>
#else
#include <errno.h>
#include <unistd.h>
#endif

#include <string.h>

static void _clientEventHandler(Event event, Connection* source, Client* client) {
    EnqueueEventItem(client->queue, event, source);
}

static void _connection(Client* client) {
    client->connected = false;
    client->connection = NewConnection(client->pool, client->address, false);
    ProbeSize(AIO4C_PROBE_CONNECTION_COUNT, 1);
    ConnectionAddSystemHandler(client->connection, AIO4C_INIT_EVENT, aio4c_connection_handler(_clientEventHandler), aio4c_connection_handler_arg(client), true);
    ConnectionAddSystemHandler(client->connection, AIO4C_CONNECTING_EVENT, aio4c_connection_handler(_clientEventHandler), aio4c_connection_handler_arg(client), true);
    ConnectionAddSystemHandler(client->connection, AIO4C_CONNECTED_EVENT, aio4c_connection_handler(_clientEventHandler), aio4c_connection_handler_arg(client), true);
    ConnectionAddSystemHandler(client->connection, AIO4C_CLOSE_EVENT, aio4c_connection_handler(_clientEventHandler), aio4c_connection_handler_arg(client), true);
    ConnectionAddSystemHandler(client->connection, AIO4C_FREE_EVENT, aio4c_connection_handler(_clientEventHandler), aio4c_connection_handler_arg(client), true);
    ConnectionAddHandler(client->connection, AIO4C_INIT_EVENT, aio4c_connection_handler(client->handler), aio4c_connection_handler_arg(client->handlerArg), true);
    ConnectionAddHandler(client->connection, AIO4C_CONNECTED_EVENT, aio4c_connection_handler(client->handler), aio4c_connection_handler_arg(client->handlerArg), true);
    ConnectionAddHandler(client->connection, AIO4C_INBOUND_DATA_EVENT, aio4c_connection_handler(client->handler), aio4c_connection_handler_arg(client->handlerArg), false);
    ConnectionAddHandler(client->connection, AIO4C_WRITE_EVENT, aio4c_connection_handler(client->handler), aio4c_connection_handler_arg(client->handlerArg), false);
    ConnectionAddHandler(client->connection, AIO4C_CLOSE_EVENT, aio4c_connection_handler(client->handler), aio4c_connection_handler_arg(client->handlerArg), true);
    ConnectionAddHandler(client->connection, AIO4C_FREE_EVENT, aio4c_connection_handler(client->handler), aio4c_connection_handler_arg(client->handlerArg), true);
    ConnectionInit(client->connection);
}

static aio4c_bool_t _clientInit(Client* client) {
    char* pipeName = aio4c_malloc(strlen(client->name) + 1);

    client->pool = NewBufferPool(client->bufferSize);
    if (pipeName != NULL) {
        snprintf(pipeName, strlen(client->name) + 1, "%s", client->name);
    }

    client->reader = NewReader(pipeName, client->bufferSize);

    if (client->reader == NULL) {
        return false;
    }

    _connection(client);

    Log(AIO4C_LOG_LEVEL_DEBUG, "started with tid 0x%08lx", client->thread->id);

    return true;
}

static aio4c_bool_t _clientRun(Client* client) {
    QueueItem item;
    aio4c_bool_t closedForError = false;
    Connection* connection;

    memset(&item, 0, sizeof(QueueItem));

    while (Dequeue(client->queue, &item, true)) {
        switch (item.type) {
            case AIO4C_QUEUE_ITEM_EXIT:
                return false;
            case AIO4C_QUEUE_ITEM_EVENT:
                connection = (Connection*)item.content.event.source;
                switch (item.content.event.type) {
                    case AIO4C_INIT_EVENT:
                        Log(AIO4C_LOG_LEVEL_DEBUG, "connection %s initialized", client->address->string);
                        ConnectionConnect(connection);
                        break;
                    case AIO4C_CONNECTING_EVENT:
                        Log(AIO4C_LOG_LEVEL_DEBUG, "finishing connection to %s", client->address->string);
                        ConnectionFinishConnect(connection);
                        if (connection->state != AIO4C_CONNECTION_STATE_CLOSED) {
                            ReaderManageConnection(client->reader, connection);
                        }
                        break;
                    case AIO4C_CONNECTED_EVENT:
                        Log(AIO4C_LOG_LEVEL_INFO, "connection established with success on %s", client->address->string);
                        client->connected = true;
                        client->retryCount = 0;
                        break;
                    case AIO4C_CLOSE_EVENT:
                        closedForError = (connection)->closedForError;

                        if (!client->connected || ConnectionNoMoreUsed(connection, AIO4C_CONNECTION_OWNER_CLIENT)) {
                            FreeConnection(&connection);
                        }

                        if (closedForError) {
                            if (client->retryCount < client->retries) {
                                client->retryCount++;
                                Log(AIO4C_LOG_LEVEL_WARN, "connection with %s lost, retrying (%d/%d) in %d seconds...", client->address->string, client->retryCount, client->retries, client->interval);
                                ProbeTimeStart(AIO4C_TIME_PROBE_IDLE);
#ifdef AIO4C_WIN32
                                Sleep(client->interval * 1000);
#else /* AIO4C_WIN32 */
                                sleep(client->interval);
#endif /* AIO4C_WIN32 */
                                ProbeTimeEnd(AIO4C_TIME_PROBE_IDLE);
                                ProbeSize(AIO4C_PROBE_CONNECTION_COUNT, -1);
                                _connection(client);
                            } else {
                                Log(AIO4C_LOG_LEVEL_ERROR, "retried too many times to connect %s, giving up", client->address->string);
                                client->exiting = true;
                            }
                        } else {
                            Log(AIO4C_LOG_LEVEL_INFO, "disconnecting from %s", client->address->string);
                            client->exiting = true;
                        }
                        break;
                    case AIO4C_FREE_EVENT:
                        if (client->exiting) {
                            return false;
                        }
                        break;
                    default:
                        break;
                }
                break;
            default:
                break;
        }
    }

    return true;
}

static void _clientExit(Client* client) {
    if (client->reader != NULL) {
        ReaderEnd(client->reader);
    }

    FreeQueue(&client->queue);
    FreeBufferPool(&client->pool);
    FreeAddress(&client->address);
    Log(AIO4C_LOG_LEVEL_DEBUG, "exited");
}

Client* NewClient(int clientIndex, AddressType type, char* address, aio4c_port_t port, int retries, int retryInterval, int bufferSize, void (*handler)(Event,Connection*,void*), void* handlerArg) {
    Client* client = NULL;
    ErrorCode code = AIO4C_ERROR_CODE_INITIALIZER;

    if ((client = aio4c_malloc(sizeof(Client))) == NULL) {
#ifndef AIO4C_WIN32
        code.error = errno;
#else /* AIO4C_WIN32 */
        code.source = AIO4C_ERRNO_SOURCE_SYS;
#endif /* AIO4C_WIN32 */
        code.size = sizeof(Client);
        code.type = "Client";
        Raise(AIO4C_LOG_LEVEL_ERROR, AIO4C_ALLOC_ERROR_TYPE, AIO4C_ALLOC_ERROR, &code);
        return NULL;
    }

    client->name       = aio4c_malloc(10);
    if (client->name != NULL) {
        snprintf(client->name, 10, "client%03d", clientIndex);
    }
    client->address    = NewAddress(type, address, port);
    client->retries    = retries;
    client->interval   = retryInterval;
    client->handler    = handler;
    client->handlerArg = handlerArg;
    client->retryCount = 0;
    client->connection = NULL;
    client->pool       = NULL;
    client->reader     = NULL;
    client->queue      = NewQueue();
    client->thread     = NULL;
    client->bufferSize = bufferSize;
    client->connected  = false;
    client->exiting    = false;

    NewThread(client->name,
            aio4c_thread_init(_clientInit),
            aio4c_thread_run(_clientRun),
            aio4c_thread_exit(_clientExit),
            aio4c_thread_arg(client),
            &client->thread);

    if (client->thread == NULL) {
        FreeQueue(&client->queue);
        FreeAddress(&client->address);
        if (client->name != NULL) {
            aio4c_free(client->name);
        }
        aio4c_free(client);
        return NULL;
    }

    return client;
}

Connection* ClientGetConnection(Client* client) {
    return client->connection;
}

void ClientEnd(Client* client) {
    if (client->thread != NULL) {
        ThreadJoin(client->thread);
    }

    if (client->name != NULL) {
        aio4c_free(client->name);
    }

    aio4c_free(client);
}

