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
    char hbuf[NI_MAXHOST], sbuf[NI_MAXSERV];

    memset(hbuf, 0, NI_MAXHOST * sizeof(char));
    memset(sbuf, 0, NI_MAXSERV * sizeof(char));

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
            if (getnameinfo((struct sockaddr*)ipv4, sizeof(struct sockaddr_in), hbuf, sizeof(hbuf), sbuf, sizeof(sbuf), NI_NUMERICHOST | NI_NUMERICSERV) == 0) {
                if ((pAddress->string = aio4c_malloc((strlen(hbuf) + strlen(sbuf) + 2) * sizeof(char))) != NULL) {
                    snprintf(pAddress->string, strlen(hbuf) + strlen(sbuf) + 2, "%s:%s", hbuf, sbuf);
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
            if (getnameinfo((struct sockaddr*)ipv6, sizeof(struct sockaddr_in6), hbuf, sizeof(hbuf), sbuf, sizeof(sbuf), NI_NUMERICHOST | NI_NUMERICSERV) == 0) {
                if ((pAddress->string = aio4c_malloc((strlen(hbuf) + strlen(sbuf) + 4) * sizeof(char))) != NULL) {
                    snprintf(pAddress->string, strlen(hbuf) + strlen(sbuf) + 4, "[%s]:%s", hbuf, sbuf);
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

