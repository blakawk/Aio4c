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
#ifndef __AIO4C_ADDRESS_H__
#define __AIO4C_ADDRESS_H__

#include <aio4c/types.h>

typedef enum e_AddressType {
    AIO4C_ADDRESS_IPV4 = 0,
    AIO4C_ADDRESS_IPV6 = 1,
#ifndef AIO4C_WIN32
    AIO4C_ADDRESS_UNIX = 2,
    AIO4C_ADDRESS_MAX = 3
#else /* AIO4C_WIN32 */
    AIO4C_ADDRESS_MAX = 2
#endif /* AIO4C_WIN32 */
} AddressType;

typedef struct s_Address {
    char*            string;
    AddressType      type;
    aio4c_addr_t*    address;
    aio4c_size_t     size;
} Address;

extern AIO4C_API Address* NewAddress(AddressType type, char* address, aio4c_port_t port);

extern AIO4C_API void FreeAddress(Address** address);

#endif
