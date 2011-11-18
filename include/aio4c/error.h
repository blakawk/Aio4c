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
 * @file aio4c/error.h
 * @brief Provides error handling capacity.
 *
 * @author blakawk
 */
#ifndef __AIO4C_ERROR_H__
#define __AIO4C_ERROR_H__

#include <aio4c/buffer.h>
#include <aio4c/condition.h>
#include <aio4c/connection.h>
#include <aio4c/lock.h>
#include <aio4c/log.h>
#include <aio4c/selector.h>
#include <aio4c/thread.h>

/**
 * @enum Error
 * @brief Handled error codes.
 */
typedef enum e_Error {
    AIO4C_NO_ERROR = 0,                        /**< No error */
    AIO4C_SOCKET_ERROR = 1,                    /**< Socket creation error */
    AIO4C_FCNTL_ERROR = 2,                     /**< Fcntl error */
    AIO4C_GETSOCKOPT_ERROR = 3,                /**< Getsockopt error */
    AIO4C_SETSOCKOPT_ERROR = 4,                /**< Setsockopt error */
    AIO4C_CONNECT_ERROR = 5,                   /**< Connection establishment error */
    AIO4C_FINISH_CONNECT_ERROR = 6,            /**< Connection establishment error */
    AIO4C_CONNECTION_DISCONNECTED = 7,         /**< Disconnection */
#ifdef AIO4C_HAVE_POLL
    AIO4C_POLL_ERROR = 8,                      /**< Poll error */
#else /* AIO4C_HAVE_POLL */
    AIO4C_SELECT_ERROR = 8,                    /**< Select error */
#endif /* AIO4C_HAVE_POLL */
    AIO4C_READ_ERROR = 9,                      /**< Network read error */
    AIO4C_WRITE_ERROR = 10,                    /**< Network write error */
    AIO4C_BUFFER_OVERFLOW_ERROR = 11,          /**< Buffer overflow error */
    AIO4C_BUFFER_UNDERFLOW_ERROR = 12,         /**< Buffer underflow error */
    AIO4C_CONNECTION_STATE_ERROR = 13,         /**< Connection state invalid error */
    AIO4C_EVENT_HANDLER_ERROR = 14,            /**< Event handler error */
    AIO4C_THREAD_LOCK_INIT_ERROR = 15,         /**< Thread lock init error */
    AIO4C_THREAD_LOCK_TAKE_ERROR = 16,         /**< Thread lock take error */
    AIO4C_THREAD_LOCK_RELEASE_ERROR = 17,      /**< Thread lock release error */
    AIO4C_THREAD_LOCK_DESTROY_ERROR = 18,      /**< Thread lock destroy error */
    AIO4C_THREAD_CONDITION_INIT_ERROR = 19,    /**< Thread condition init error */
    AIO4C_THREAD_CONDITION_WAIT_ERROR = 20,    /**< Thread condition wait error */
    AIO4C_THREAD_CONDITION_NOTIFY_ERROR = 21,  /**< Thread condition notify error */
    AIO4C_THREAD_CONDITION_DESTROY_ERROR = 22, /**< Thread condition destroy error */
    AIO4C_THREAD_SELECTOR_INIT_ERROR = 23,     /**< Thread selector init error */
    AIO4C_THREAD_SELECTOR_SELECT_ERROR = 24,   /**< Thread select error */
    AIO4C_THREAD_SELECTOR_WAKE_UP_ERROR = 25,  /**< Thread wake up error */
    AIO4C_THREAD_CREATE_ERROR = 26,            /**< Thread creation error */
    AIO4C_THREAD_CANCEL_ERROR = 27,            /**< Thread cancellation error */
    AIO4C_THREAD_JOIN_ERROR = 28,              /**< Thread join error */
    AIO4C_ALLOC_ERROR = 29,                    /**< Memory allocation error */
    AIO4C_JNI_FIELD_ERROR = 30,                /**< JNI field access error */
    AIO4C_MAX_ERRORS = 31                      /**< Number of errors */
} Error;

/**
 * @enum ErrorType
 * @brief Handled error types.
 */
typedef enum e_ErrorType {
    AIO4C_BUFFER_ERROR_TYPE = 0,           /**< The error is Buffer related */
    AIO4C_CONNECTION_ERROR_TYPE = 1,       /**< The error is Connection related */
    AIO4C_CONNECTION_STATE_ERROR_TYPE = 2, /**< The error is related to the Connection state */
    AIO4C_SOCKET_ERROR_TYPE = 3,           /**< The error is related to a socket operation */
    AIO4C_EVENT_ERROR_TYPE = 4,            /**< The error is related to an Event */
    AIO4C_THREAD_ERROR_TYPE = 5,           /**< The error is related to a Thread operation */
    AIO4C_THREAD_LOCK_ERROR_TYPE = 6,      /**< The error is related to a Lock operation */
    AIO4C_THREAD_CONDITION_ERROR_TYPE = 7, /**< The error is related to a Condition operation */
    AIO4C_THREAD_SELECTOR_ERROR_TYPE = 8,  /**< The error is related to a Selector operation */
    AIO4C_ALLOC_ERROR_TYPE = 9,            /**< The error is a memory allocation error */
    AIO4C_JNI_ERROR_TYPE = 10,             /**< The error is related to a JNI operation */
    AIO4C_MAX_ERROR_TYPES = 11             /**< Number of error types */
} ErrorType;

#ifdef AIO4C_WIN32

/**
 * @enum ErrnoSource
 * @brief Defines where to retrieve the OS error code.
 */
typedef enum e_ErrnoSource {
    AIO4C_ERRNO_SOURCE_NONE = 0, /**< Default value */
    AIO4C_ERRNO_SOURCE_SYS = 1,  /**< The error code should be obtained via GetLastError() */
    AIO4C_ERRNO_SOURCE_WSA = 2,  /**< The error code should be obtained via WSAGetLastError() */
    AIO4C_ERRNO_SOURCE_SOE = 3,  /**< The error code is given */
    AIO4C_ERRNO_SOURCE_MAX = 4   /**< Number fo error code sources */
} ErrnoSource;

#endif

/**
 * @struct ErrorCode
 * @brief Holds error context information.
 */
/**
 * @def __AIO4C_ERROR_CODE_DEFINED__
 * @brief Defined when ErrorCode type is defined.
 */
#ifndef __AIO4C_ERROR_CODE_DEFINED__
#define __AIO4C_ERROR_CODE_DEFINED__
typedef struct s_ErrorCode {
#ifndef AIO4C_WIN32
    int             error;      /**< The errno value of the failed system call */
#else /* AIO4C_WIN32 */
    ErrnoSource     source;     /**< How to retrieve the errno value of the failed system call */
    int             soError;    /**< The errno value to use when ErrnoSource is AIO4C_ERRNO_SOURCE_SOE */
#endif /* AIO4C_WIN32 */
    Buffer*         buffer;     /**< Pointer to the Buffer that triggered the error */
    Thread*         thread;     /**< Pointer to the Thread that triggered the error */
    Condition*      condition;  /**< Pointer to the Condition that triggered the error */
    Lock*           lock;       /**< Pointer to the Lock that triggered the error */
    Selector*       selector;   /**< Pointer to the Selector that triggered the error */
    Connection*     connection; /**< Pointer to the Connection that triggered the error */
    ConnectionState expected;   /**< Expected state of the Connection that triggered the error */
    int             size;       /**< Requested allocation size that triggered the error */
    char*           type;       /**< Name of the type to allocate that triggered the error */
    char*           field;      /**< Field's name of the class that was not found */
    char*           signature;  /**< Field's type signature that was not found */
} ErrorCode;
#endif /* __AIO4C_ERROR_CODE_DEFINED__ */

/**
 * @def AIO4C_ERROR_CODE_INITIALIZER
 * @brief Initializes an ErrorCode.
 *
 * Allows to initialize all fields of ErrorCode structure.
 */
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

/**
 * @def Raise(level,type,error,code)
 * @brief Wrapper to the _Raise function.
 *
 * Expand the filename and line number of the caller using preprocessor macros
 * __FILE__ and __LINE__.
 *
 * @param level
 *   The LogLevel of the log message associated with the error
 * @param type
 *   The ErrorType of the error to raise
 * @param error
 *   The Error to raise
 * @param code
 *   Pointer to the ErrorCode information associated with the Error
 *
 * @see void _Raise(char*,int,LogLevel,ErrorType,Error,ErrorCode*)
 */
#define Raise(level,type,error,code) \
    _Raise(__FILE__, __LINE__, level, type, error, code)

/**
 * @fn void _Raise(char*,int,LogLevel,ErrorType,Error,ErrorCode*)
 * @brief Raises an Error logging a message describing it.
 *
 * This function gather all debugging information needed according to the ErrorCode and format
 * the appropriate message to be sent to the Log subsystem.
 *
 * @param file
 *   The filename where the error was raised
 * @param line
 *   The line number where the error was raised
 * @param level
 *   The LogLevel to log the error message with
 * @param type
 *   The ErrorType of the error to log
 * @param error
 *   The Error to log
 * @param code
 *   Pointer to the ErrorCode information associated with the Error
 *
 * @see void Log(LogLevel,char*,...)
 */
extern AIO4C_API void _Raise(char* file, int line, LogLevel level, ErrorType type, Error error, ErrorCode* code);

#endif /* __AIO4C_ERROR_H__ */
