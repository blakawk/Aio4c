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
#include <aio4c/error.h>

#include <aio4c/condition.h>
#include <aio4c/connection.h>
#include <aio4c/lock.h>
#include <aio4c/log.h>
#include <aio4c/selector.h>
#include <aio4c/thread.h>

#ifdef AIO4C_WIN32
#include <winbase.h>
#include <winsock2.h>
#endif

#include <stdio.h>
#include <string.h>

char* ErrorStrings[AIO4C_MAX_ERRORS] = {
    "success",           /* AIO4C_NO_ERROR */
    "socket",            /* AIO4C_SOCKET_ERROR */
    "fcntl",             /* AIO4C_FCNTL_ERROR */
    "getsockopt",        /* AIO4C_GETSOCKOPT_ERROR */
    "setsockopt",        /* AIO4C_SETSOCKOPT_ERROR */
    "connect",           /* AIO4C_CONNECT_ERROR */
    "finishing connect", /* AIO4C_FINISH_CONNECT_ERROR */
    "disconnected",      /* AIO4C_CONNECTION_DISCONNECTED */
#ifdef AIO4C_HAVE_POLL
    "poll",              /* AIO4C_POLL_ERROR */
#else /* AIO4C_HAVE_POLL */
    "select",            /* AIO4C_SELECT_ERROR */
#endif /* AIO4C_HAVE_POLL */
    "read",              /* AIO4C_READ_ERROR */
    "write",             /* AIO4C_WRITE_ERROR */
    "overflow",          /* AIO4C_BUFFER_OVERFLOW_ERROR */
    "underflow",         /* AIO4C_BUFFER_UNDERFLOW_ERROR */
    "invalid state",     /* AIO4C_CONNECTION_STATE_ERROR */
    "event handler add", /* AIO4C_EVENT_HANDLER_ERROR */
    "init",              /* AIO4C_THREAD_LOCK_INIT_ERROR */
    "take",              /* AIO4C_THREAD_LOCK_TAKE_ERROR */
    "release",           /* AIO4C_THREAD_LOCK_RELEASE_ERROR */
    "destroy",           /* AIO4C_THREAD_LOCK_DESTROY_ERROR */
    "init",              /* AIO4C_THREAD_CONDITION_INIT_ERROR */
    "wait",              /* AIO4C_THREAD_CONDITION_WAIT_ERROR */
    "notify",            /* AIO4C_THREAD_CONDITION_NOTIFY_ERROR */
    "destroy",           /* AIO4C_THREAD_CONDITION_DESTROY_ERROR */
    "init",              /* AIO4C_THREAD_SELECTOR_INIT_ERROR */
    "wait",              /* AIO4C_THREAD_SELECTOR_WAIT_ERROR */
    "wake up",           /* AIO4C_THREAD_SELECTOR_WAKEUP_ERROR */
    "create",            /* AIO4C_THREAD_CREATE_ERROR */
    "cancel",            /* AIO4C_THREAD_CANCEL_ERROR */
    "join",              /* AIO4C_THREAD_JOIN_ERROR */
    "allocate",          /* AIO4C_ALLOC_ERROR */
    "retrieve field"     /* AIO4C_JNI_FIELD_ERROR */
};

void _Raise(char* file, int line, LogLevel level, ErrorType type, Error error, ErrorCode* code) {
    char* errorMessage = NULL;
    long errorCode = 0;

#ifndef AIO4C_WIN32
    errorCode = (long)code->error;
    errorMessage = strerror(code->error);
#else /* AIO4C_WIN32 */
    switch (code->source) {
        case AIO4C_ERRNO_SOURCE_WSA:
            errorCode = WSAGetLastError();
            break;
        case AIO4C_ERRNO_SOURCE_SYS:
            errorCode = GetLastError();
            break;
        case AIO4C_ERRNO_SOURCE_SOE:
            errorCode = code->soError;
            break;
        default:
            break;
    }

    if (FormatMessage(FORMAT_MESSAGE_ALLOCATE_BUFFER | FORMAT_MESSAGE_FROM_SYSTEM,
            NULL, errorCode, MAKELANGID(LANG_ENGLISH, SUBLANG_ENGLISH_US), (LPTSTR)&errorMessage, 0, NULL) != 0) {
        errorMessage[strlen(errorMessage) - 2] = '\0';
    }
#endif /* AIO4C_WIN32 */

    switch (type) {
        case AIO4C_BUFFER_ERROR_TYPE:
            Log(level, "[E:%02d,T:%02d] %s:%d: %s on buffer %p[cap:%d,lim:%d,pos:%d]", error, type, file, line, ErrorStrings[error],
                    (void*)BufferGetBytes(code->buffer), BufferGetCapacity(code->buffer), BufferGetLimit(code->buffer), BufferGetPosition(code->buffer));
            break;
        case AIO4C_CONNECTION_ERROR_TYPE:
            if (error == AIO4C_CONNECTION_DISCONNECTED) {
                Log(level, "connection %s disconnected", code->connection->string);
            } else {
                Log(level, "[E:%02d,T:%02d] %s:%d: %s for connection %s[%s]: [%08ld] %s", error, type, file, line, ErrorStrings[error],
                        code->connection->string, ConnectionStateString[code->connection->state],
                        errorCode, errorMessage);
            }
            break;
        case AIO4C_EVENT_ERROR_TYPE:
            Log(level, "[E:%02d,T:%02d] %s:%d: %s for connection %s[%s]",
                    error, type, file, line, ErrorStrings[error],
                    code->connection->string, ConnectionStateString[code->connection->state]);
            break;
        case AIO4C_THREAD_ERROR_TYPE:
            Log(level, "[E:%02d,T:%02d] %s:%d: %s for thread %s[i:0x%lx,s:%s]: [%08ld] %s", error, type, file, line, ErrorStrings[error],
                    ThreadGetName(code->thread), ThreadGetId(code->thread), ThreadStateString[ThreadGetState(code->thread)],
                    errorCode, errorMessage);
            break;
        case AIO4C_THREAD_LOCK_ERROR_TYPE:
            if (LockGetOwner(code->lock) != NULL) {
                Log(level, "[E:%02d,T:%02d] %s:%d: %s lock %p[s:%s,o:%s[i:0x%lx,s:%s]]: [%08ld] %s", error, type, file, line, ErrorStrings[error],
                        (void*)code->lock, LockStateString[LockGetState(code->lock)], ThreadGetName(LockGetOwner(code->lock)),
                        ThreadGetId(LockGetOwner(code->lock)), ThreadStateString[ThreadGetState(LockGetOwner(code->lock))], errorCode, errorMessage);
            } else {
                Log(level, "[E:%02d,T:%02d] %s:%d: %s lock %p[s:%s,o:none]: [%08ld] %s", error, type, file, line, ErrorStrings[error],
                        (void*)code->lock, LockStateString[LockGetState(code->lock)], errorCode, errorMessage);
            }
            break;
        case AIO4C_THREAD_CONDITION_ERROR_TYPE:
            if (ConditionGetOwner(code->condition) != NULL) {
                Log(level, "[E:%02d,T:%02d] %s:%d: %s condition %p[s:%s,o:%s[i:0x%lx,s:%s]]: [%08ld] %s", error, type, file, line, ErrorStrings[error],
                        (void*)code->condition, ConditionStateString[ConditionGetState(code->condition)], ThreadGetName(ConditionGetOwner(code->condition)),
                        ThreadGetId(ConditionGetOwner(code->condition)), ThreadStateString[ThreadGetState(ConditionGetOwner(code->condition))], errorCode, errorMessage);
            } else {
                Log(level, "[E:%02d,T:%02d] %s:%d: %s condition %p[s:%s,o:none]: [%08ld] %s", error, type, file, line, ErrorStrings[error],
                        (void*)code->condition, ConditionStateString[ConditionGetState(code->condition)], errorCode, errorMessage);
            }
            break;
        case AIO4C_THREAD_SELECTOR_ERROR_TYPE:
#ifdef AIO4C_HAVE_POLL
#ifdef AIO4C_HAVE_PIPE
            Log(level, "[E:%02d,T:%02d] %s:%d: %s selector %p[p:%d/%d]: [%08ld] %s", error, type, file, line, ErrorStrings[error],
                    (void*)code->selector, code->selector->numPolls, code->selector->maxPolls,
                    errorCode, errorMessage);
#else /* AIO4C_HAVE_PIPE */
            Log(level, "[E:%02d,T:%02d] %s:%d: %s selector %p[p:%d/%d,u:%d]: [%08ld] %s", error, type, file, line, ErrorStrings[error],
                    (void*)code->selector, code->selector->numPolls, code->selector->maxPolls,
                    code->selector->port, errorCode, errorMessage);
#endif /* AIO4C_HAVE_PIPE */
#else /* AIO4C_HAVE_POLL */
#ifdef AIO4C_HAVE_PIPE
            Log(level, "[E:%02d,T:%02d] %s:%d: %s selector %p: [%08ld] %s", error, type, file, line, ErrorStrings[error],
                    (void*)code->selector,
                    errorCode, errorMessage);
#else /* AIO4C_HAVE_PIPE */
            Log(level, "[E:%02d,T:%02d] %s:%d: %s selector %p[u:%d]: [%08ld] %s", error, type, file, line, ErrorStrings[error],
                    (void*)code->selector, code->selector->port,
                    errorCode, errorMessage);
#endif /* AIO4C_HAVE_PIPE */
#endif /* AIO4C_HAVE_POLL */
            break;
        case AIO4C_ALLOC_ERROR_TYPE:
            Log(level, "[E:%02d,T:%02d] %s:%d: %s %s of size %d bytes: [%08ld] %s", error, type, file, line, ErrorStrings[error],
                    code->type, code->size, errorCode, errorMessage);
            break;
        case AIO4C_CONNECTION_STATE_ERROR_TYPE:
            Log(level, "[E:%02d,T:%02d] %s:%d: %s for connection %s[%s], expected %s", error, type, file, line, ErrorStrings[error],
                    code->connection->string, ConnectionStateString[code->connection->state], ConnectionStateString[code->expected]);
            break;
        case AIO4C_SOCKET_ERROR_TYPE:
            Log(level, "[E:%02d,T:%02d] %s:%d: %s: [%08ld] %s", error, type, file, line, ErrorStrings[error], errorCode, errorMessage);
            break;
        case AIO4C_JNI_ERROR_TYPE:
            switch (error) {
                case AIO4C_JNI_FIELD_ERROR:
                    Log(level, "[E:%02d,T:%02d] %s:%d: %s %s with signature %s failed", error, type, file, line, ErrorStrings[error], code->field, code->signature);
                    break;
                default:
                    Log(level, "[E:%02d,T:%02d] %s:%d: unknown JNI error", error, type, file, line);
                    break;
            }
            break;
        default:
            break;
    }

#ifdef AIO4C_WIN32
    if (errorMessage != NULL) {
        LocalFree(errorMessage);
    }
#endif
}

