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
#ifndef __AIO4C_ERROR_H__
#define __AIO4C_ERROR_H__

#include <aio4c/buffer.h>
#include <aio4c/connection.h>
#include <aio4c/log.h>
#include <aio4c/thread.h>

typedef enum e_Error {
    AIO4C_NO_ERROR,
    AIO4C_SOCKET_ERROR,
    AIO4C_FCNTL_ERROR,
    AIO4C_GETSOCKOPT_ERROR,
    AIO4C_SETSOCKOPT_ERROR,
    AIO4C_CONNECT_ERROR,
    AIO4C_FINISH_CONNECT_ERROR,
    AIO4C_CONNECTION_DISCONNECTED,
#ifdef AIO4C_HAVE_POLL
    AIO4C_POLL_ERROR,
#else /* AIO4C_HAVE_POLL */
    AIO4C_SELECT_ERROR,
#endif /* AIO4C_HAVE_POLL */
    AIO4C_READ_ERROR,
    AIO4C_WRITE_ERROR,
    AIO4C_BUFFER_OVERFLOW_ERROR,
    AIO4C_BUFFER_UNDERFLOW_ERROR,
    AIO4C_CONNECTION_STATE_ERROR,
    AIO4C_EVENT_HANDLER_ERROR,
    AIO4C_THREAD_LOCK_INIT_ERROR,
    AIO4C_THREAD_LOCK_TAKE_ERROR,
    AIO4C_THREAD_LOCK_RELEASE_ERROR,
    AIO4C_THREAD_LOCK_DESTROY_ERROR,
    AIO4C_THREAD_CONDITION_INIT_ERROR,
    AIO4C_THREAD_CONDITION_WAIT_ERROR,
    AIO4C_THREAD_CONDITION_NOTIFY_ERROR,
    AIO4C_THREAD_CONDITION_DESTROY_ERROR,
    AIO4C_THREAD_SELECTOR_INIT_ERROR,
    AIO4C_THREAD_SELECTOR_SELECT_ERROR,
    AIO4C_THREAD_SELECTOR_WAKE_UP_ERROR,
    AIO4C_THREAD_CREATE_ERROR,
    AIO4C_THREAD_CANCEL_ERROR,
    AIO4C_THREAD_JOIN_ERROR,
    AIO4C_ALLOC_ERROR,
    AIO4C_JNI_FIELD_ERROR,
    AIO4C_MAX_ERRORS
} Error;

typedef enum e_ErrorType {
    AIO4C_BUFFER_ERROR_TYPE,
    AIO4C_CONNECTION_ERROR_TYPE,
    AIO4C_CONNECTION_STATE_ERROR_TYPE,
    AIO4C_SOCKET_ERROR_TYPE,
    AIO4C_EVENT_ERROR_TYPE,
    AIO4C_THREAD_ERROR_TYPE,
    AIO4C_THREAD_LOCK_ERROR_TYPE,
    AIO4C_THREAD_CONDITION_ERROR_TYPE,
    AIO4C_THREAD_SELECTOR_ERROR_TYPE,
    AIO4C_ALLOC_ERROR_TYPE,
    AIO4C_JNI_ERROR_TYPE,
    AIO4C_MAX_ERROR_TYPES
} ErrorType;

#ifdef AIO4C_WIN32

typedef enum e_ErrnoSource {
    AIO4C_ERRNO_SOURCE_NONE,
    AIO4C_ERRNO_SOURCE_SYS,
    AIO4C_ERRNO_SOURCE_WSA,
    AIO4C_ERRNO_SOURCE_SOE,
    AIO4C_ERRNO_SOURCE_MAX
} ErrnoSource;

#endif

#ifndef __AIO4C_BUFFER_DEFINED__
#define __AIO4C_BUFFER_DEFINED__
typedef struct s_Buffer Buffer;
#endif

#ifndef __AIO4C_CONNECTION_DEFINED__
#define __AIO4C_CONNECTION_DEFINED__
typedef struct s_Connection Connection;
#endif

typedef struct s_ErrorCode {
#ifndef AIO4C_WIN32
    int             error;
#else /* AIO4C_WIN32 */
    ErrnoSource     source;
    int             soError;
#endif /* AIO4C_WIN32 */
    Buffer*         buffer;
    Thread*         thread;
    Condition*      condition;
    Lock*           lock;
    Selector*       selector;
    Connection*     connection;
    ConnectionState expected;
    int             size;
    char*           type;
    char*           field;
    char*           signature;
} ErrorCode;

#ifndef AIO4C_WIN32

#define AIO4C_ERROR_CODE_INITIALIZER {         \
    .error      = 0,                           \
    .buffer     = NULL,                        \
    .thread     = NULL,                        \
    .condition  = NULL,                        \
    .lock       = NULL,                        \
    .selector   = NULL,                        \
    .connection = NULL,                        \
    .size       = 0,                           \
    .type       = NULL,                        \
    .expected   = AIO4C_CONNECTION_STATE_NONE, \
    .field      = NULL,                        \
    .signature  = NULL                         \
}

#else /* AIO4C_WIN32 */

#define AIO4C_ERROR_CODE_INITIALIZER {         \
    .source     = AIO4C_ERRNO_SOURCE_NONE,     \
    .soError    = 0,                           \
    .buffer     = NULL,                        \
    .thread     = NULL,                        \
    .condition  = NULL,                        \
    .lock       = NULL,                        \
    .selector   = NULL,                        \
    .connection = NULL,                        \
    .size       = 0,                           \
    .type       = NULL,                        \
    .expected   = AIO4C_CONNECTION_STATE_NONE, \
    .field      = NULL,                        \
    .signature  = NULL                         \
}

#endif /* AIO4C_WIN32 */

#define Raise(level,type,error,code) \
    _Raise(__FILE__, __LINE__, level, type, error, code)

extern void _Raise(char* file, int line, LogLevel level, ErrorType type, Error error, ErrorCode* code);

#endif
