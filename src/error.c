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
#include <aio4c/error.h>

#include <aio4c/log.h>

#ifndef AIO4C_WIN32

#include <string.h>

#else /* AIO4C_WIN32 */

#include <winbase.h>
#include <winsock2.h>

#endif

#include <stdio.h>

char* ErrorStrings[AIO4C_MAX_ERRORS] = {
    "success",           /* AIO4C_NO_ERROR */
    "socket",            /* AIO4C_SOCKET_ERROR */
    "fcntl",             /* AIO4C_FCNTL_ERROR */
    "getsockopt",        /* AIO4C_GETSOCKOPT_ERROR */
    "setsockopt",        /* AIO4C_SETSOCKOPT_ERROR */
    "connect",           /* AIO4C_CONNECT_ERROR */
    "poll",              /* AIO4C_POLL_ERROR */
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
    "allocate"           /* AIO4C_ALLOC_ERROR */
};

void _Raise(char* file, int line, LogLevel level, ErrorType type, Error error, ErrorCode* code) {
    char* errorMessage = NULL;
    int errorCode = 0;

#ifndef AIO4C_WIN32
    errorCode = code->error;
    errorMessage = strerror(code->error);
#else /* AIO4C_WIN32 */
    switch (error.source) {
        case AIO4C_ERRNO_SOURCE_WSA:
            errorCode = WSAGetLastError();
            break;
        case AIO4C_ERRNO_SOURCE_SYS:
            errorCode = GetLastError();
            break;
        default:
            break;
    }

    FormatMessage(FORMAT_MESSAGE_ALLOCATE_BUFFER | FORMAT_MESSAGE_FROM_SYSTEM,
            NULL, errorCode, 0, (LPTSTR)&errorMessage, 0, NULL);
#endif /* AIO4C_WIN32 */

    switch (type) {
        case AIO4C_BUFFER_ERROR_TYPE:
            Log(NULL, level, "%s:%d: %s on buffer %p[cap:%d,lim:%d,pos:%d]", file, line, ErrorStrings[error],
                    (void*)code->buffer->data, code->buffer->size, code->buffer->limit, code->buffer->position);
            break;
        case AIO4C_CONNECTION_ERROR_TYPE:
            Log(NULL, level, "%s:%d: %s for connection %s[%s]: [0x%08x] %s", file, line, ErrorStrings[error],
                    code->connection->string, ConnectionStateString[code->connection->state],
                    errorCode, errorMessage);
            break;
        case AIO4C_EVENT_ERROR_TYPE:
            Log(NULL, level, "%s:%d: %s for connection %s[%s], handlers:{sys:[I:%d/%d,CIP:%d/%d,C:%d/%d,R:%d/%d,ID:%d/%d,W:%d/%d,OD:%d/%d,CL:%d/%d],usr:[I:%d/%d,CIP:%d/%d,C:%d/%d,R:%d/%d,ID:%d/%d,W:%d/%d,OD:%d/%d,CL:%d/%d]}",
                    file, line, ErrorStrings[error],
                    code->connection->string, ConnectionStateString[code->connection->state],
                    code->connection->systemHandlers->handlersCount[AIO4C_INIT_EVENT],
                    code->connection->systemHandlers->maxHandlers[AIO4C_INIT_EVENT],
                    code->connection->systemHandlers->handlersCount[AIO4C_CONNECTING_EVENT],
                    code->connection->systemHandlers->maxHandlers[AIO4C_CONNECTING_EVENT],
                    code->connection->systemHandlers->handlersCount[AIO4C_CONNECTED_EVENT],
                    code->connection->systemHandlers->maxHandlers[AIO4C_CONNECTED_EVENT],
                    code->connection->systemHandlers->handlersCount[AIO4C_READ_EVENT],
                    code->connection->systemHandlers->maxHandlers[AIO4C_READ_EVENT],
                    code->connection->systemHandlers->handlersCount[AIO4C_INBOUND_DATA_EVENT],
                    code->connection->systemHandlers->maxHandlers[AIO4C_INBOUND_DATA_EVENT],
                    code->connection->systemHandlers->handlersCount[AIO4C_WRITE_EVENT],
                    code->connection->systemHandlers->maxHandlers[AIO4C_WRITE_EVENT],
                    code->connection->systemHandlers->handlersCount[AIO4C_OUTBOUND_DATA_EVENT],
                    code->connection->systemHandlers->maxHandlers[AIO4C_OUTBOUND_DATA_EVENT],
                    code->connection->systemHandlers->handlersCount[AIO4C_CLOSE_EVENT],
                    code->connection->systemHandlers->maxHandlers[AIO4C_CLOSE_EVENT],
                    code->connection->userHandlers->handlersCount[AIO4C_INIT_EVENT],
                    code->connection->userHandlers->maxHandlers[AIO4C_INIT_EVENT],
                    code->connection->userHandlers->handlersCount[AIO4C_CONNECTING_EVENT],
                    code->connection->userHandlers->maxHandlers[AIO4C_CONNECTING_EVENT],
                    code->connection->userHandlers->handlersCount[AIO4C_CONNECTED_EVENT],
                    code->connection->userHandlers->maxHandlers[AIO4C_CONNECTED_EVENT],
                    code->connection->userHandlers->handlersCount[AIO4C_READ_EVENT],
                    code->connection->userHandlers->maxHandlers[AIO4C_READ_EVENT],
                    code->connection->userHandlers->handlersCount[AIO4C_INBOUND_DATA_EVENT],
                    code->connection->userHandlers->maxHandlers[AIO4C_INBOUND_DATA_EVENT],
                    code->connection->userHandlers->handlersCount[AIO4C_WRITE_EVENT],
                    code->connection->userHandlers->maxHandlers[AIO4C_WRITE_EVENT],
                    code->connection->userHandlers->handlersCount[AIO4C_OUTBOUND_DATA_EVENT],
                    code->connection->userHandlers->maxHandlers[AIO4C_OUTBOUND_DATA_EVENT],
                    code->connection->userHandlers->handlersCount[AIO4C_CLOSE_EVENT],
                    code->connection->userHandlers->maxHandlers[AIO4C_CLOSE_EVENT]);
            break;
        case AIO4C_THREAD_ERROR_TYPE:
            Log(NULL, level, "%s:%d: %s for thread %s[r:%p,i:0x%lx,s:%s]: [0x%08x] %s", file, line, ErrorStrings[error],
                    code->thread->name, *(void**)&code->thread->run, (unsigned long)code->thread->id, ThreadStateString[code->thread->state],
                    errorCode, errorMessage);
            break;
        case AIO4C_THREAD_LOCK_ERROR_TYPE:
            if (code->lock->owner != NULL) {
                Log(NULL, level, "%s:%d: %s lock %p[s:%s,o:%s[r:%p,i:0x%lx,s:%s]]: [0x%08x] %s", file, line, ErrorStrings[error],
                        (void*)code->lock, LockStateString[code->lock->state], code->lock->owner->name, *(void**)&code->lock->owner->run,
                        (unsigned long)code->lock->owner->id, ThreadStateString[code->lock->owner->state], errorCode, errorMessage);
            } else {
                Log(NULL, level, "%s:%d: %s lock %p[s:%s,o:none]: [0x%08x] %s", file, line, ErrorStrings[error],
                        (void*)code->lock, LockStateString[code->lock->state], errorCode, errorMessage);
            }
            break;
        case AIO4C_THREAD_CONDITION_ERROR_TYPE:
            if (code->condition->owner != NULL) {
                Log(NULL, level, "%s:%d: %s condition %p[s:%s,o:%s[r:%p,i:0x%lx,s:%s]]: [0x%08x] %s", file, line, ErrorStrings[error],
                        (void*)code->condition, ConditionStateString[code->condition->state], code->condition->owner->name, *(void**)&code->condition->owner->run,
                        (unsigned long)code->condition->owner->id, ThreadStateString[code->condition->owner->state], errorCode, errorMessage);
            } else {
                Log(NULL, level, "%s:%d: %s condition %p[s:%s,o:none]: [0x%08x] %s", file, line, ErrorStrings[error],
                        (void*)code->condition, ConditionStateString[code->condition->state], errorCode, errorMessage);
            }
            break;
        case AIO4C_THREAD_SELECTOR_ERROR_TYPE:
#ifndef AIO4C_WIN32
            Log(NULL, level, "%s:%d: %s selector %p[k:%d/%d,p:%d/%d]: [0x%08x] %s", file, line, ErrorStrings[error],
                    (void*)code->selector, code->selector->numKeys, code->selector->maxKeys, code->selector->numPolls, code->selector->maxPolls,
                    errorCode, errorMessage);
#else /* AIO4C_WIN32 */
            Log(NULL, level, "%s:%d: %s selector %p[k:%d/%d,p:%d/%d,u:%d]: [0x%08x] %s", file, line, ErrorStrings[error],
                    (void*)code->selector, code->selector->numKeys, code->selector->maxKeys, code->selector->numPolls, code->selector->maxPolls,
                    code->selector->port, errorCode, errorMessage);
#endif /* AIO4C_WIN32 */
            break;
        case AIO4C_ALLOC_ERROR_TYPE:
            Log(NULL, level, "%s:%d: %s %s of size %d bytes: [0x%08x] %s", file, line, ErrorStrings[error],
                    code->type, code->size, errorCode, errorMessage);
            break;
        case AIO4C_SOCKET_ERROR_TYPE:
            Log(NULL, level, "%s:%d: %s: [0x%08x] %s", file, line, ErrorStrings[error], errorCode, errorMessage);
            break;
        default:
            break;
    }

#ifdef AIO4C_WIN32
    LocalFree(errorMessage);
#endif
}

