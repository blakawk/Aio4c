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
#include <aio4c/address.h>

#include <aio4c/alloc.h>
#include <aio4c/types.h>

#ifndef AIO4C_WIN32
#include <arpa/inet.h>
#include <netdb.h>
#include <netinet/in.h>
#include <netinet/ip.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <sys/un.h>
#else
#include <winsock2.h>
#include <ws2tcpip.h>
#endif

#include <stdlib.h>
#include <string.h>

#ifndef AIO4C_WIN32
#ifndef UNIX_PATH_MAX
#define UNIX_PATH_MAX 108
#endif
#endif

static void ResolveIP(AddressType type, char* address, struct sockaddr* ip) {
    struct addrinfo hints;
    struct addrinfo* result = NULL;

    memset(&hints, 0, sizeof(struct addrinfo));

    hints.ai_family   = AF_UNSPEC;
    hints.ai_socktype = SOCK_STREAM;

    switch (type) {
        case AIO4C_ADDRESS_IPV4:
            hints.ai_family = AF_INET;
            break;
        case AIO4C_ADDRESS_IPV6:
            hints.ai_family = AF_INET6;
            break;
        default:
            break;
    }

    if (getaddrinfo(address, NULL, &hints, &result) == 0) {
        switch (type) {
            case AIO4C_ADDRESS_IPV4:
                memcpy(&(((struct sockaddr_in*)ip)->sin_addr), &(((struct sockaddr_in*)result->ai_addr)->sin_addr), sizeof(struct in_addr));
                break;
            case AIO4C_ADDRESS_IPV6:
                memcpy(&(((struct sockaddr_in6*)ip)->sin6_addr), &(((struct sockaddr_in6*)result->ai_addr)->sin6_addr), sizeof(struct in6_addr));
                break;
            default:
                break;
        }
    }

    freeaddrinfo(result);
}

Address* NewAddress(AddressType type, char* address, aio4c_port_t port) {
    Address* pAddress = NULL;
    struct sockaddr_in* ipv4 = NULL;
    struct sockaddr_in6* ipv6 = NULL;
#ifndef AIO4C_WIN32
    struct sockaddr_un* un = NULL;
#endif
    aio4c_bool_t addPort = false;
    aio4c_size_t addrLen = 0, stringLen = 0;

    if ((pAddress = aio4c_malloc(sizeof(Address))) == NULL) {
        return NULL;
    }

    pAddress->type = type;

    switch(type) {
        case AIO4C_ADDRESS_IPV4:
            pAddress->size = sizeof(struct sockaddr_in);
            break;
        case AIO4C_ADDRESS_IPV6:
            pAddress->size = sizeof(struct sockaddr_in6);
            break;
#ifndef AIO4C_WIN32
        case AIO4C_ADDRESS_UNIX:
            pAddress->size = sizeof(struct sockaddr_un);
            break;
#endif
        default:
            break;
    }

    if ((pAddress->address = aio4c_malloc(pAddress->size)) == NULL) {
        FreeAddress(&pAddress);
        return NULL;
    }

    switch(type) {
        case AIO4C_ADDRESS_IPV4:
            ipv4 = (struct sockaddr_in*)pAddress->address;
            ipv4->sin_family = AF_INET;
            ipv4->sin_port = htons(port);
            ResolveIP(type, address, (struct sockaddr*)ipv4);
            if ((pAddress->string = aio4c_malloc(INET_ADDRSTRLEN * sizeof(char))) != NULL) {
                if (getnameinfo((struct sockaddr*)ipv4, sizeof(struct sockaddr_in), pAddress->string, INET_ADDRSTRLEN, NULL, 0, NI_NUMERICHOST) == 0) {
                    addrLen = strlen(pAddress->string);
                    addPort = true;
                }
            }
            break;
        case AIO4C_ADDRESS_IPV6:
            ipv6 = (struct sockaddr_in6*)pAddress->address;
            ipv6->sin6_family = AF_INET6;
            ipv6->sin6_port = htons(port);
            ipv6->sin6_flowinfo = 0;
            ipv6->sin6_scope_id = 0;
            ResolveIP(type, address, (struct sockaddr*)ipv6);
            if ((pAddress->string = aio4c_malloc((INET6_ADDRSTRLEN + 1) * sizeof(char))) != NULL) {
                if (getnameinfo((struct sockaddr*)ipv6, sizeof(struct sockaddr_in6), &pAddress->string[1], INET6_ADDRSTRLEN, NULL, 0, NI_NUMERICHOST) == 0) {
                    pAddress->string[0] = '[';
                    addrLen = strlen(pAddress->string);
                    addPort = true;
                }
            }
            break;
#ifndef AIO4C_WIN32
        case AIO4C_ADDRESS_UNIX:
            un = (struct sockaddr_un*)pAddress->address;
            un->sun_family = AF_UNIX;
            strncpy(un->sun_path, address, UNIX_PATH_MAX);
            break;
#endif
        default:
            FreeAddress(&pAddress);
            break;
    }

    if (addPort) {
        if (type == AIO4C_ADDRESS_IPV4) {
            stringLen = snprintf(&pAddress->string[addrLen], 0, ":%u", port);
            pAddress->string = aio4c_realloc(pAddress->string, (addrLen + stringLen + 1) * sizeof(char));
            snprintf(&pAddress->string[addrLen], stringLen + 1, ":%u", port);
        } else if (type == AIO4C_ADDRESS_IPV6) {
            stringLen = snprintf(&pAddress->string[addrLen], 0, "]:%u", port);
            pAddress->string = aio4c_realloc(pAddress->string, (addrLen + stringLen + 1) * sizeof(char));
            snprintf(&pAddress->string[addrLen], stringLen + 1, "]:%u", port);
        }
    }

    return pAddress;
}

void FreeAddress(Address** address) {
    Address* pAddress = NULL;

    if (address != NULL && (pAddress = *address) != NULL) {
        if (pAddress->address != NULL) {
            aio4c_free(pAddress->address);
        }

        if (pAddress->string != NULL) {
            aio4c_free(pAddress->string);
        }

        aio4c_free(pAddress);
        *address = NULL;
    }
}

