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
    AIO4C_NO_ERROR = 0,
    AIO4C_SOCKET_ERROR = 1,
    AIO4C_FCNTL_ERROR = 2,
    AIO4C_GETSOCKOPT_ERROR = 3,
    AIO4C_SETSOCKOPT_ERROR = 4,
    AIO4C_CONNECT_ERROR = 5,
    AIO4C_FINISH_CONNECT_ERROR = 6,
    AIO4C_CONNECTION_DISCONNECTED = 7,
#ifdef AIO4C_HAVE_POLL
    AIO4C_POLL_ERROR = 8,
#else /* AIO4C_HAVE_POLL */
    AIO4C_SELECT_ERROR = 8,
#endif /* AIO4C_HAVE_POLL */
    AIO4C_READ_ERROR = 9,
    AIO4C_WRITE_ERROR = 10,
    AIO4C_BUFFER_OVERFLOW_ERROR = 11,
    AIO4C_BUFFER_UNDERFLOW_ERROR = 12,
    AIO4C_CONNECTION_STATE_ERROR = 13,
    AIO4C_EVENT_HANDLER_ERROR = 14,
    AIO4C_THREAD_LOCK_INIT_ERROR = 15,
    AIO4C_THREAD_LOCK_TAKE_ERROR = 16,
    AIO4C_THREAD_LOCK_RELEASE_ERROR = 17,
    AIO4C_THREAD_LOCK_DESTROY_ERROR = 18,
    AIO4C_THREAD_CONDITION_INIT_ERROR = 19,
    AIO4C_THREAD_CONDITION_WAIT_ERROR = 20,
    AIO4C_THREAD_CONDITION_NOTIFY_ERROR = 21,
    AIO4C_THREAD_CONDITION_DESTROY_ERROR = 22,
    AIO4C_THREAD_SELECTOR_INIT_ERROR = 23,
    AIO4C_THREAD_SELECTOR_SELECT_ERROR = 24,
    AIO4C_THREAD_SELECTOR_WAKE_UP_ERROR = 25,
    AIO4C_THREAD_CREATE_ERROR = 26,
    AIO4C_THREAD_CANCEL_ERROR = 27,
    AIO4C_THREAD_JOIN_ERROR = 28,
    AIO4C_ALLOC_ERROR = 29,
    AIO4C_JNI_FIELD_ERROR = 30,
    AIO4C_MAX_ERRORS = 31
} Error;

typedef enum e_ErrorType {
    AIO4C_BUFFER_ERROR_TYPE = 0,
    AIO4C_CONNECTION_ERROR_TYPE = 1,
    AIO4C_CONNECTION_STATE_ERROR_TYPE = 2,
    AIO4C_SOCKET_ERROR_TYPE = 3,
    AIO4C_EVENT_ERROR_TYPE = 4,
    AIO4C_THREAD_ERROR_TYPE = 5,
    AIO4C_THREAD_LOCK_ERROR_TYPE = 6,
    AIO4C_THREAD_CONDITION_ERROR_TYPE = 7,
    AIO4C_THREAD_SELECTOR_ERROR_TYPE = 8,
    AIO4C_ALLOC_ERROR_TYPE = 9,
    AIO4C_JNI_ERROR_TYPE = 10,
    AIO4C_MAX_ERROR_TYPES = 11
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

extern AIO4C_API void _Raise(char* file, int line, LogLevel level, ErrorType type, Error error, ErrorCode* code);

#endif
