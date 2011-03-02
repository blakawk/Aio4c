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
#ifndef __AIO4C_TYPES_H__
#define __AIO4C_TYPES_H__

#ifndef AIO4C_WIN32
#include <netinet/in.h>
#include <poll.h>
#else
#include <winsock2.h>
#endif

#include <pthread.h>
#include <stdio.h>
#include <sys/types.h>

typedef unsigned char aio4c_byte_t;

typedef enum e_bool {
    false = 0,
    true = 1
} aio4c_bool_t;

typedef unsigned int aio4c_size_t;

typedef unsigned int aio4c_position_t;

typedef struct sockaddr aio4c_addr_t;

typedef short aio4c_port_t;

#define INET_PORTSTRLEN 5

typedef int aio4c_socket_t;

typedef FILE aio4c_file_t;

typedef pthread_mutex_t aio4c_lock_t;

typedef pthread_cond_t aio4c_cond_t;

typedef struct pollfd aio4c_poll_t;

typedef int aio4c_pipe_t[2];

#ifndef socklen_t
typedef int socklen_t;
#endif

#define AIO4C_PIPE_READ (short)0

#define AIO4C_PIPE_WRITE (short)1

#ifdef AIO4C_WIN32
#ifdef AIO4C_BUILD
#define __aio4c_dll __declspec(dllexport)
#else
#define __aio4c_dll __declspec(dllimport)
#endif
#else
#define __aio4c_dll
#endif

#endif
