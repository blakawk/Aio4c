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
 * @file aio4c/address.h
 * @brief Module handling Addresses.
 *
 * This module is used to handle several addresses managed by the library.
 *
 * @author blakawk
 */
#ifndef __AIO4C_ADDRESS_H__
#define __AIO4C_ADDRESS_H__

#include <aio4c/types.h>

/**
 * @typedef AddressType
 * @brief Represents an address type.
 *
 * Used in several modules to defines the kind of TCP address to use.
 */
typedef enum e_AddressType {
    AIO4C_ADDRESS_IPV4 = 0, /**< IPv4 address */
    AIO4C_ADDRESS_IPV6 = 1, /**< IPv6 address */
#ifndef AIO4C_WIN32
    AIO4C_ADDRESS_UNIX = 2, /**< Unix address */
#endif /* AIO4C_WIN32 */
} AddressType;

/**
 * @typedef Address
 *
 * @brief Holds IP address information.
 *
 * Type used to enclose informations needed to handle different addresses
 * kind in POSIX.
 */
/**
 * @def __AIO4C_ADDRESS_DEFINED__
 * @brief Defined if Address type has been defined.
 *
 * @see Address
 */
#ifndef __AIO4C_ADDRESS_DEFINED__
#define __AIO4C_ADDRESS_DEFINED__
typedef struct s_Address Address;
#endif /* __AIO4C_ADDRESS_DEFINED__ */

/**
 * @fn Address* NewAddress(AddressType,char*,aio4c_port_t)
 * @brief Allocates a new Address.
 *
 * Initializes Address members using parameters, resolves the host name if
 * needed according to the provided AddressType.
 *
 * @param type
 *   The kind of address to create.
 * @param address
 *   The address's host name or numeric notation according to the AddressType.
 * @param port
 *   The port number.
 * @return
 *   A pointer to the created Address.
 *
 * @see AddressType
 */
extern AIO4C_API Address* NewAddress(AddressType type, char* address, aio4c_port_t port);

/**
 * @fn aio4c_addr_t* AddressGetAddr(Address*)
 * @brief Gets this address POSIX structure.
 *
 * According to this AddressType, returns:
 *   - a pointer to a sockaddr_in structure if AIO4C_ADDRESS_IPV4,
 *   - a pointer to a sockaddr_in6 structure if AIO4C_ADDRESS_IPV6,
 *   - on Unix platforms, a pointer to a sockaddr_un structure if AIO4C_ADDRESS_UNIX.
 *
 * @param address
 *   A pointer to the address to retrieve the member from.
 * @return
 *   A pointer to this address POSIX structure.
 *
 * @see aio4c_addr_t
 */
extern AIO4C_API aio4c_addr_t* AddressGetAddr(Address* address);

/**
 * @fn int AddressGetAddrSize(Address*)
 * @brief Gets this address POSIX structure size.
 *
 * According to this AddressType, returns:
 *   - sizeof(struct sockaddr_in) if AIO4C_ADDRESS_IPV4,
 *   - sizeof(struct sockaddr_in6) if AIO4C_ADDRESS_IPV6,
 *   - on Unix platforms, sizeof(struct sockaddr_un) if AIO4C_ADDRESS_UNIX.
 *
 * @param address
 *   A pointer to the address to retrieve the member from.
 * @return
 *   The size of this address POSIX structure.
 */
extern AIO4C_API int AddressGetAddrSize(Address* address);

/**
 * @fn char* AddressGetString(Address*)
 * @brief Gets this address string representation
 *
 * According to this AddressType, returns the numeric representation of the IP,
 * followed by a colon, and the port number.
 *
 * @param address
 *   A pointer to the address to retrieve the member from.
 * @return
 *   The string representation of this address.
 */
extern AIO4C_API char* AddressGetString(Address* address);

/**
 * @fn AddressType AddressGetType(Address*)
 * @brief Gets this address AddressType.
 *
 * @param address
 *   A pointer to the address to retrieve the member from.
 * @return
 *   The AddressType of this address.
 *
 * @see AddressType
 */
extern AIO4C_API AddressType AddressGetType(Address* address);

/**
 * @fn void FreeAddress(Address**)
 * @brief Frees an address.
 *
 * Once the Address is freed, the pointed pointer is set to NULL.
 *
 * @param address
 *   A pointer to an Address pointer returned by NewAddress.
 *
 * @see NewAddress
 */
extern AIO4C_API void FreeAddress(Address** address);

#endif /* __AIO4C_ADDRESS_H__ */
