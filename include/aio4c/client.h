/*
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
/**
 * @file aio4c/client.h
 * @brief Provides Client.
 *
 * This module defines Client, which represents a TCP client that will connect
 * to a TCP server.
 *
 * @author blakawk
 */
#ifndef __AIO4C_CLIENT_H__
#define __AIO4C_CLIENT_H__

#include <aio4c/address.h>
#include <aio4c/connection.h>
#include <aio4c/types.h>

/**
 * @struct s_Client
 * @brief Represents a Thread that will manage TCP client connections.
 *
 * A Client will try to establish a TCP connection to a TCP server using the
 * parameters given on NewClient() function. Once the Connection has been
 * established with succcess, the Client will handle input data from the
 * network as well as output data events using one pipe composed of one
 * Reader, one Worker and one Writer.
 *
 * If the TCP connection is lost because the server closed it unexpectedly or
 * for any other reason which is not a normal disconnection, the Client will
 * try to reconnect to the server if it was parameterized to do it.
 *
 * The Client terminates only when the Connection cannot be established after
 * the maximum number of retries.
 *
 * @see NewClient()
 * @see Reader
 * @see Worker
 * @see Writer
 */
/**
 * @def __AIO4C_CLIENT_DEFINED__
 * @brief Defined if Client type has been defined.
 *
 * @see Client
 */
#ifndef __AIO4C_CLIENT_DEFINED__
#define __AIO4C_CLIENT_DEFINED__
typedef struct s_Client Client;
#endif /* __AIO4C_CLIENT_DEFINED__ */

/**
 * @typedef ClientHandlerData
 * @brief Data passed to ClientHandler.
 *
 * This should be used to point out additionnal data that will be needed by the
 * ClientHandler when handling a Connection event.
 *
 * @see ClientHandler
 */
/**
 * @def __AIO4C_CLIENT_HANDLER_DATA_DEFINED__
 * @brief Defined if ClientHandlerData type has been defined.
 *
 * @see ClientHandlerData
 */
#ifndef __AIO4C_CLIENT_HANDLER_DATA_DEFINED__
#define __AIO4C_CLIENT_HANDLER_DATA_DEFINED__
typedef struct s_ClientHandlerData* ClientHandlerData;
#endif /* __AIO4C_CLIENT_HANDLER_DATA_DEFINED__ */

/**
 * @typedef ClientHandler
 * @brief Handler used by Client to handle Connection events.
 *
 * This handler will be called for each event that will occurs en the
 * Client's Connection, with as a parameter the ClientHandlerData indicated
 * on Client creation.
 *
 * It takes three parameters:
 *   - Event: the event to handle,
 *   - Connection*: pointer to the Connection on which the Event occured,
 *   - ClientHandlerData: the handler data.
 *
 * @see Connection
 * @see Event
 * @see NewClient()
 */
/**
 * @def __AIO4C_CLIENT_HANDLER_DEFINED__
 * @brief Defined if ClientHandler type has been defined.
 *
 * @see ClientHandler
 */
#ifndef __AIO4C_CLIENT_HANDLER_DEFINED__
#define __AIO4C_CLIENT_HANDLER_DEFINED__
typedef void (*ClientHandler)(Event,Connection*,ClientHandlerData);
#endif /* __AIO4C_CLIENT_HANDLER_DEFINED__ */

/**
 * @fn Client* NewClient(int,AddressType,char*,aio4c_port_t,int,int,int,ClientHandler,ClientHandlerData)
 * @brief Creates a Client.
 *
 * Allocates and initializes Client data structures, but does not start it.
 * If you want to start the Client, uses ClientStart(Client*) function.
 *
 * @param clientIndex
 *   An index identifying the Client. Only used for debug purpose.
 * @param type
 *   The kind of AddressType the Client will handle.
 * @param address
 *   A string representating the host the Client will connect to.
 * @param port
 *   The port number the Client will connect to.
 * @param retries
 *   The maximum number of retries when the Connection cannot be established
 *   for any reason, or if the Connection is closed unexpectedly. 0 means that
 *   the Client will never retry.
 * @param retryInterval
 *   The interval in seconds between two retries.
 * @param bufferSize
 *   The capacity that will be used for Buffer's allocation.
 * @param handler
 *   The handler that will be used to handle Connection's events.
 * @param arg
 *   The argument passed when calling ClientHandler.
 * @return
 *   A pointer to a Client structure, or NULL if the Client allocation failed.
 *
 * @see AddressType
 * @see Client
 * @see Buffer
 */
extern AIO4C_API Client* NewClient(
        int clientIndex,
        AddressType type,
        char* address,
        aio4c_port_t port,
        int retries,
        int retryInterval,
        int bufferSize,
        ClientHandler handler,
        ClientHandlerData arg);

/**
 * @fn aio4c_bool_t ClientStart(Client*)
 * @brief Starts a Client.
 *
 * Starts the Client thread and wait for it to initialize.
 *
 * @param client
 *   A pointer to the Client to start.
 * @return
 *   true if the Client was started and initialized with success, false if not.
 *
 * @see ThreadStart(Thread)
 */
extern AIO4C_API aio4c_bool_t ClientStart(Client* client);

/**
 * @fn Connection* ClientGetConnection(Client*)
 * @brief Retrieves the Client's Connection.
 *
 * @param client
 *   A pointer to the Client to retrieve the Connection for.
 * @return
 *   A pointer to the Client's Connection, or NULL if no Connection has been
 *   established successfully yet.
 */
extern AIO4C_API Connection* ClientGetConnection(Client* client);

/**
 * @fn void ClientEnd(Client*)
 * @brief Waits for a Client to terminate.
 *
 * Join the Client thread and frees it. After this function returned, the
 * Client pointer will no longer be valid.
 *
 * @param client
 *   A pointer to the Client to stop.
 */
extern AIO4C_API void ClientEnd(Client* client);

#endif /* __AIO4C_CLIENT_H__ */
