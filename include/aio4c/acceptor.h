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
 * @file aio4c/acceptor.h
 * @brief Module defining Acceptors.
 *
 * This module defines Acceptor, which is used in Server to process incoming
 * Connections.
 *
 * @author blakawk
 */
#ifndef __AIO4C_ACCEPTOR_H__
#define __AIO4C_ACCEPTOR_H__

#include <aio4c/address.h>
#include <aio4c/connection.h>

/**
 * @typedef Acceptor
 * @brief Represents a thread that accept connections from a Server.
 *
 * The Acceptor will wait for new connection of a Server's master socket, and
 * creates new Connection instance using the provided Connection factory.
 * It will also manage the Server's pipes, a pipe being composed of:
 *   - a Reader that manages incoming data on Server's Connections,
 *   - a Worker that perform data processing on incoming data,
 *   - and a Writer that sends data over a Connection.
 *
 * @see Server
 * @see Connection
 * @see Reader
 * @see Worker
 * @see Writer
 */
#ifndef __AIO4C_ACCEPTOR_DEFINED__
#define __AIO4C_ACCEPTOR_DEFINED__
typedef struct s_Acceptor Acceptor;
#endif /* __AIO4C_ACCEPTOR_DEFINED__ */

/**
 * @fn Acceptor* NewAcceptor(char*,Address*,Connection*,int)
 * @brief Creates an Acceptor.
 *
 * When this function returns a value different from NULL, the Acceptor is
 * ready to process incoming connection, meaning that the thread has been
 * started and initialized.
 *
 * @param name
 *   The Acceptor name (used only for debug purpose).
 * @param address
 *   The Address the Acceptor has to listen on.
 * @param factory
 *   The Connection factory used to create Connections when a new connection
 *   is accepted by the Acceptor.
 * @param nbPipes
 *   The number of pipes to use to manage connections.
 * @return
 *   A pointer to the Acceptor structure.
 *
 * @see Address
 * @see NewConnectionFactory(BufferPool,void*(*)(Connection*,void*),void*)
 */
extern AIO4C_API Acceptor* NewAcceptor(char* name, Address* address, Connection* factory, int nbPipes);

/**
 * @fn void AcceptorEnd(Acceptor*)
 * @brief Terminates an Acceptor.
 *
 * Set the Acceptor Thread running state to false and wait for it to terminate.
 *
 * @param acceptor
 *   The Acceptor to terminate.
 */
extern AIO4C_API void AcceptorEnd(Acceptor* acceptor);

#endif /* __AIO4C_ACCEPTOR_H__ */
