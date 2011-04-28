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
#ifndef __AIO4C_TYPES_H__
#define __AIO4C_TYPES_H__

#ifndef AIO4C_WIN32

#include <netdb.h>
#include <netinet/in.h>
#include <poll.h>

#else /* AIO4C_WIN32 */

#include <winsock2.h>
#include <windows.h>

#endif /* AIO4C_WIN32 */

#include <stdio.h>
#include <sys/types.h>

#define AIO4C_PIPE_READ (short)0
#define AIO4C_PIPE_WRITE (short)1

#ifndef AIO4C_API
#define AIO4C_API
#endif /* AIO4C_API */

#ifndef NI_MAXHOST
#define NI_MAXHOST 1025
#endif /* NI_MAXHOST */

#ifndef NI_MAXSERV
#define NI_MAXSERV 32
#endif /* NI_MAXSERV */

typedef unsigned char aio4c_byte_t;

typedef enum e_bool {
    false = 0,
    true = 1
} aio4c_bool_t;

typedef unsigned int aio4c_size_t;

typedef unsigned int aio4c_position_t;

typedef struct sockaddr aio4c_addr_t;

typedef int aio4c_port_t;

#ifndef AIO4C_WIN32

typedef int aio4c_socket_t;

#else /* AIO4C_WIN32 */

typedef SOCKET aio4c_socket_t;

typedef int socklen_t;

#endif /* AIO4C_WIN32 */

typedef FILE aio4c_file_t;

#ifdef AIO4C_HAVE_POLL
typedef struct pollfd aio4c_poll_t;
#endif /* AIO4C_HAVE_POLL */

typedef aio4c_socket_t aio4c_pipe_t[2];

#endif
