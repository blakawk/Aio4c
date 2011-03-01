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
#include <aio4c/client.h>

#include <aio4c/address.h>
#include <aio4c/alloc.h>
#include <aio4c/buffer.h>
#include <aio4c/log.h>
#include <aio4c/reader.h>
#include <aio4c/stats.h>
#include <aio4c/thread.h>
#include <aio4c/types.h>

#include <string.h>
#include <unistd.h>

static void _clientEventHandler(Event event, Connection* source, Client* client) {
    if (event == CONNECTED_EVENT) {
        ReaderManageConnection(client->reader, source);
        client->connected = true;
        client->retryCount = 0;
    }

    EnqueueEventItem(client->queue, event, source);
}

static void _connection(Client* client) {
    client->connection = NewConnection(client->pool, client->address, false);
    ConnectionAddSystemHandler(client->connection, INIT_EVENT, aio4c_connection_handler(_clientEventHandler), aio4c_connection_handler_arg(client), true);
    ConnectionAddSystemHandler(client->connection, CONNECTING_EVENT, aio4c_connection_handler(_clientEventHandler), aio4c_connection_handler_arg(client), true);
    ConnectionAddSystemHandler(client->connection, CONNECTED_EVENT, aio4c_connection_handler(_clientEventHandler), aio4c_connection_handler_arg(client), true);
    ConnectionAddSystemHandler(client->connection, CLOSE_EVENT, aio4c_connection_handler(_clientEventHandler), aio4c_connection_handler_arg(client), true);
    ConnectionAddHandler(client->connection, INIT_EVENT, aio4c_connection_handler(client->handler), aio4c_connection_handler_arg(client->handlerArg), true);
    ConnectionAddHandler(client->connection, CONNECTED_EVENT, aio4c_connection_handler(client->handler), aio4c_connection_handler_arg(client->handlerArg), true);
    ConnectionAddHandler(client->connection, INBOUND_DATA_EVENT, aio4c_connection_handler(client->handler), aio4c_connection_handler_arg(client->handlerArg), false);
    ConnectionAddHandler(client->connection, WRITE_EVENT, aio4c_connection_handler(client->handler), aio4c_connection_handler_arg(client->handlerArg), false);
    ConnectionAddHandler(client->connection, CLOSE_EVENT, aio4c_connection_handler(client->handler), aio4c_connection_handler_arg(client->handlerArg), true);
    ConnectionInit(client->connection);
}

static void _clientInit(Client* client) {
    Log(client->thread, INFO, "started");

    client->pool = NewBufferPool(2, client->bufferSize);
    client->reader = NewReader("client-reader", client->bufferSize);
    _connection(client);
}

static aio4c_bool_t _clientRun(Client* client) {
    QueueItem item;

    memset(&item, 0, sizeof(QueueItem));

    while (Dequeue(client->queue, &item, true)) {
        switch (item.type) {
            case EXIT:
                return false;
            case EVENT:
                switch (item.content.event.type) {
                    case INIT_EVENT:
                        Log(client->thread, DEBUG, "connection %s initialized", client->address->string);
                        ConnectionConnect((Connection*)item.content.event.source);
                        break;
                    case CONNECTING_EVENT:
                        Log(client->thread, DEBUG, "finishing connection to %s", client->address->string);
                        ConnectionFinishConnect((Connection*)item.content.event.source);
                        break;
                    case CONNECTED_EVENT:
                        Log(client->thread, INFO, "connection established with success on %s", client->address->string);
                        break;
                    case CLOSE_EVENT:
                        if (client->retryCount < client->retries) {
                            client->retryCount++;
                            ProbeTimeStart(TIME_PROBE_IDLE);
                            sleep(client->interval);
                            ProbeTimeEnd(TIME_PROBE_IDLE);
                            Log(client->thread, WARN, "connection with %s lost, retrying (%d/%d)", client->address->string, client->retryCount, client->retries);
                            if (!client->connected) {
                                FreeConnection(&client->connection);
                            } else {
                                client->connected = false;
                            }
                            _connection(client);
                        } else {
                            Log(client->thread, ERROR, "retried too many times to connect %s, giving up", client->address->string);
                            FreeConnection(&client->connection);
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
    ReaderEnd(client->reader);
    FreeQueue(&client->queue);
    FreeBufferPool(&client->pool);
    FreeAddress(&client->address);
    aio4c_free(client);
}

extern Thread* NewClient(AddressType type, char* address, aio4c_port_t port, int retries, int retryInterval, int bufferSize, void (*handler)(Event,Connection*,void*), void* handlerArg) {
    Client* client = NULL;

    if ((client = aio4c_malloc(sizeof(Client))) == NULL) {
        return NULL;
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

    client->thread     = NewThread("client",
            aio4c_thread_handler(_clientInit),
            aio4c_thread_run(_clientRun),
            aio4c_thread_handler(_clientExit),
            aio4c_thread_arg(client));

    return client->thread;
}

