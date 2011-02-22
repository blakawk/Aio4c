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
#ifndef __AIO4C_ACCEPTOR_H__
#define __AIO4C_ACCEPTOR_H__

#include <aio4c/address.h>
#include <aio4c/connection.h>
#include <aio4c/reader.h>
#include <aio4c/thread.h>
#include <aio4c/types.h>

typedef struct s_Acceptor {
    Thread*        thread;
    Address*       address;
    aio4c_socket_t socket;
    Reader*        reader;
    Selector*      selector;
    SelectionKey*  key;
    Connection*    factory;
    Queue*         queue;
} Acceptor;

extern Acceptor* NewAcceptor(char* name, AddressType type, char* address, aio4c_port_t port, Connection* factory);

extern void AcceptorEnd(Acceptor* acceptor);

#endif
