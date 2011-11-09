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

#ifdef HAVE_CONFIG_H
# include "config.h"
#endif /* HAVE_CONFIG_H */

#if defined(HAVE_POLL) || defined(HAVE_WSAPOLL)
# ifndef AIO4C_HAVE_POLL
#  define AIO4C_HAVE_POLL
# endif /* AIO4C_HAVE_POLL */
#endif /* HAVE_POLL || HAVE_WSAPOLL */

#if defined(HAVE_PIPE)
# ifndef AI4OC_HAVE_PIPE
#  define AIO4C_HAVE_PIPE
# endif /* AIO4C_HAVE_PIPE */
#endif /* HAVE_PIPE */

#if defined(HAVE_INITIALIZECONDITIONVARIABLE)
# ifndef AIO4C_HAVE_CONDITION
#  define AIO4C_HAVE_CONDITION
# endif /* AIO4C_HAVE_CONDITION */
#endif /* HAVE_INITIALIZECONDITIONVARIABLE */

#ifndef AIO4C_WIN32
# if defined(_WIN32) || defined(_WIN64)
#  define AIO4C_WIN32
# endif /* _WIN32 || _WIN64 */
#endif /* AIO4C_WIN32 */

#ifndef AIO4C_WIN32

#include <netdb.h>
#include <netinet/in.h>
#include <poll.h>

#else /* AIO4C_WIN32 */

#ifdef _WINSOCK2API_
# error "have to redefine FD_SETSIZE before winsock2 inclusion"
#else /* _WINSOCK2API_ */
# define FD_SETSIZE 4096
#endif /* _WINSOCK2API_ */

#include <winsock2.h>
#include <windows.h>

#endif /* AIO4C_WIN32 */

#include <stdio.h>
#include <sys/types.h>

#define AIO4C_PIPE_READ (short)0
#define AIO4C_PIPE_WRITE (short)1

#ifndef AIO4C_API
# ifdef AIO4C_WIN32
#  define AIO4C_API __declspec(dllexport)
# else /* AIO4C_WIN32 */
#  define AIO4C_API
# endif /* AIO4C_WIN32 */
#endif /* AIO4C_API */

#ifndef AIO4C_MAX_THREADS
#define AIO4C_MAX_THREADS 4096
#endif /* AIO4C_MAX_THREADS */

#if !defined(SIZEOF_VOIDP) || (SIZEOF_VOIDP == 0)
# ifndef AIO4C_P_SIZE
#  ifdef AIO4C_WIN32
#   if defined(_WIN32)
#    define AIO4C_P_SIZE 4
#   elif defined(_WIN64)
#    define AIO4C_P_SIZE 8
#   else /* _WIN64 */
#    error "cannot determine size of void*, please define AIO4C_P_SIZE for me"
#   endif /* _WIN64 */
#  else /* AIO4C_WIN32 */
#    if defined(__amd64)
#      define AIO4C_P_SIZE 8
#    else /* __amd64 */
#      define AIO4C_P_SIZE 4
#    endif /* __amd64 */
#  endif /* AIO4C_WIN32 */
# endif /* AIO4C_P_SIZE */
#else /* defined(SIZEOF_VOIDP) && (SIZEOF_VOIDP != 0) */
# ifndef AIO4C_P_SIZE
#  define AIO4C_P_SIZE SIZEOF_VOIDP
# endif /* AIO4C_P_SIZE */
#endif /* !defined(SIZEOF_VOIDP) || (SIZEOF_VOIDP == 0) */

#ifndef NI_MAXHOST
#define NI_MAXHOST 1025
#endif /* NI_MAXHOST */

#ifndef NI_MAXSERV
#define NI_MAXSERV 32
#endif /* NI_MAXSERV */

typedef unsigned char aio4c_byte_t;

#if HAVE_STDBOOL_H
# include <stdbool.h>
#else /* HAVE_STDBOOL_H */
# if !defined(HAVE__BOOL)
#  ifdef __cplusplus
typedef bool _Bool;
#  else /* __cplusplus */
typedef unsigned char _Bool;
#  endif /* __cplusplus */
# endif /* !defined(HAVE__BOOL) */
# define bool _Bool
# define false 0
# define true 1
# define __bool_true_false_are_defined 1
#endif /* HAVE_STDBOOL_H */

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
