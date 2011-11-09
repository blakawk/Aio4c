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
#include <aio4c/condition.h>

#include <aio4c/alloc.h>
#include <aio4c/error.h>
#include <aio4c/lock.h>
#include <aio4c/stats.h>
#include <aio4c/thread.h>
#include <aio4c/types.h>

#ifndef AIO4C_WIN32
#include <errno.h>
#include <pthread.h>
#else /* AIO4C_WIN32 */
#include <winbase.h>
#endif /* AIO4C_WIN32 */

char* ConditionStateString[AIO4C_COND_STATE_MAX] = {
    "DESTROYED",
    "FREE",
    "WAITED"
};

struct s_Condition {
    ConditionState state;
    Thread*        owner;
#ifndef AIO4C_WIN32
    pthread_cond_t     condition;
#else /* AIO4C_WIN32 */
#ifdef AIO4C_HAVE_CONDITION
    CONDITION_VARIABLE condition;
#else /* AIO4C_HAVE_CONDITION */
    HANDLE             mutex;
    HANDLE             event;
#endif /* AIO4C_HAVE_CONDITION */
#endif /* AIO4C_WIN32 */
};

Condition* NewCondition(void) {
    Condition* condition = NULL;
    ErrorCode code = AIO4C_ERROR_CODE_INITIALIZER;

    if ((condition = aio4c_malloc(sizeof(Condition))) == NULL) {
#ifndef AIO4C_WIN32
        code.error = errno;
#else /* AIO4C_WIN32 */
        code.source = AIO4C_ERRNO_SOURCE_SYS;
#endif /* AIO4C_WIN32 */
        code.size = sizeof(Condition);
        code.type = "Condition";
        Raise(AIO4C_LOG_LEVEL_ERROR, AIO4C_ALLOC_ERROR_TYPE, AIO4C_ALLOC_ERROR, &code);
        return NULL;
    }

    condition->owner = NULL;
    condition->state = AIO4C_COND_STATE_DESTROYED;

#ifndef AIO4C_WIN32
    if ((code.error = pthread_cond_init(&condition->condition, NULL)) != 0) {
        code.condition = condition;
        Raise(AIO4C_LOG_LEVEL_ERROR, AIO4C_THREAD_CONDITION_ERROR_TYPE, AIO4C_THREAD_CONDITION_INIT_ERROR, &code);
        aio4c_free(condition);
        return NULL;
    }
#else /* AIO4C_WIN32 */
#ifdef AIO4C_HAVE_CONDITION
    InitializeConditionVariable(&condition->condition);
#else /* AIO4C_HAVE_CONDITION */
    if ((condition->mutex = CreateMutex(NULL, FALSE, NULL)) == NULL) {
        code.source = AIO4C_ERRNO_SOURCE_SYS;
        code.condition = condition;
        Raise(AIO4C_LOG_LEVEL_ERROR, AIO4C_THREAD_CONDITION_ERROR_TYPE, AIO4C_THREAD_CONDITION_INIT_ERROR, &code);
        aio4c_free(condition);
        return NULL;
    }

    if ((condition->event = CreateEvent(NULL, TRUE, FALSE, NULL)) == NULL) {
        code.source = AIO4C_ERRNO_SOURCE_SYS;
        code.condition = condition;
        Raise(AIO4C_LOG_LEVEL_ERROR, AIO4C_THREAD_CONDITION_ERROR_TYPE, AIO4C_THREAD_CONDITION_INIT_ERROR, &code);
        CloseHandle(condition->mutex);
        aio4c_free(condition);
        return NULL;
    }
#endif /* AIO4C_HAVE_CONDITION */
#endif /* AIO4C_WIN32 */

    condition->state = AIO4C_COND_STATE_FREE;

    return condition;
}

bool _WaitCondition(char* file, int line, Condition* condition, Lock* lock) {
    ErrorCode code = AIO4C_ERROR_CODE_INITIALIZER;
    Thread* current = ThreadSelf();
    char* name = (current!=NULL)?current->name:NULL;

    condition->owner = current;
    condition->state = AIO4C_COND_STATE_WAITED;

    LockSetState(LockSetOwner(lock, NULL), AIO4C_COND_STATE_FREE);

    if (current != NULL) {
        current->state = AIO4C_THREAD_STATE_IDLE;
    }

    dthread("%s:%d: %s WAIT CONDITION %p\n", file, line, name, (void*)condition);

#ifndef AIO4C_WIN32
    if ((code.error = pthread_cond_wait(&condition->condition, LockGetMutex(lock))) != 0) {
        code.condition = condition;
        Raise(AIO4C_LOG_LEVEL_WARN, AIO4C_THREAD_CONDITION_ERROR_TYPE, AIO4C_THREAD_CONDITION_WAIT_ERROR, &code);
        condition->owner = NULL;
        condition->state = AIO4C_COND_STATE_FREE;
        if (current != NULL) {
            current->state = AIO4C_THREAD_STATE_RUNNING;
        }
        return false;
    }
#else /* AIO4C_WIN32 */
#ifdef AIO4C_HAVE_CONDITION
    if (SleepConditionVariableCS(&condition->condition, LockGetMutex(lock), INFINITE) == 0) {
        code.source = AIO4C_ERRNO_SOURCE_SYS;
        code.condition = condition;
        Raise(AIO4C_LOG_LEVEL_WARN, AIO4C_THREAD_CONDITION_ERROR_TYPE, AIO4C_THREAD_CONDITION_WAIT_ERROR, &code);
        condition->owner = NULL;
        condition->state = AIO4C_COND_STATE_FREE;
        if (current != NULL) {
            current->state = AIO4C_THREAD_STATE_RUNNING;
        }
        return false;
    }
#else /* AIO4C_HAVE_CONDITION */
    dthread("[WAIT CONDITION %p] %s about to wait for mutex\n", (void*)condition, name);
    switch (WaitForSingleObject(condition->mutex, INFINITE)) {
        case WAIT_OBJECT_0:
            dthread("[WAIT CONDITION %p] %s gained mutex\n", (void*)condition, name);
            break;
        case WAIT_ABANDONED:
            dthread("[WAIT CONDITION %p] %s abandoned mutex\n", (void*)condition, name);
            Log(AIO4C_LOG_LEVEL_WARN, "%s:%d: WaitForSingleObject: abandon", __FILE__, __LINE__);
            condition->owner = NULL;
            condition->state = AIO4C_COND_STATE_FREE;
            if (current != NULL) {
                current->state = AIO4C_THREAD_STATE_RUNNING;
            }
            return false;
        case WAIT_FAILED:
            dthread("[WAIT CONDITION %p] %s failed mutex with code 0x%08lx\n", (void*)condition, name, GetLastError());
            code.source = AIO4C_ERRNO_SOURCE_SYS;
            code.condition = condition;
            Raise(AIO4C_LOG_LEVEL_WARN, AIO4C_THREAD_CONDITION_ERROR_TYPE, AIO4C_THREAD_CONDITION_WAIT_ERROR, &code);
            condition->owner = NULL;
            condition->state = AIO4C_COND_STATE_FREE;
            if (current != NULL) {
                current->state = AIO4C_THREAD_STATE_RUNNING;
            }
            return false;
        default:
            break;
    }

    LeaveCriticalSection(LockGetMutex(lock));

    dthread("[WAIT CONDITION %p] %s about to wait for event\n", (void*)condition, name);
    switch (SignalObjectAndWait(condition->mutex, condition->event, INFINITE, FALSE)) {
        case WAIT_OBJECT_0:
            dthread("[WAIT CONDITION %p] %s received event\n", (void*)condition, name);
            break;
        case WAIT_ABANDONED:
            dthread("[WAIT CONDITION %p] %s abandoned event\n", (void*)condition, name);
            Log(AIO4C_LOG_LEVEL_WARN, "%s:%d: SignalObjectAndWait: abandon", __FILE__, __LINE__);
            condition->owner = NULL;
            condition->state = AIO4C_COND_STATE_FREE;
            if (current != NULL) {
                current->state = AIO4C_THREAD_STATE_RUNNING;
            }
            return false;
        case WAIT_FAILED:
            dthread("[WAIT CONDITION %p] %s failed event with code 0x%08lx\n", (void*)condition, name, GetLastError());
            code.source = AIO4C_ERRNO_SOURCE_SYS;
            code.condition = condition;
            Raise(AIO4C_LOG_LEVEL_WARN, AIO4C_THREAD_CONDITION_ERROR_TYPE, AIO4C_THREAD_CONDITION_WAIT_ERROR, &code);
            condition->owner = NULL;
            condition->state = AIO4C_COND_STATE_FREE;
            if (current != NULL) {
                current->state = AIO4C_THREAD_STATE_RUNNING;
            }
            return false;
        default:
            break;
    }

    if (ResetEvent(condition->event) == 0) {
        code.source = AIO4C_ERRNO_SOURCE_SYS;
        code.condition = condition;
        Raise(AIO4C_LOG_LEVEL_WARN, AIO4C_THREAD_CONDITION_ERROR_TYPE, AIO4C_THREAD_CONDITION_WAIT_ERROR, &code);
    }

    EnterCriticalSection(LockGetMutex(lock));
#endif /* AIO4C_HAVE_CONDITION */
#endif /* AIO4C_WIN32 */

    LockSetState(LockSetOwner(lock, current), AIO4C_LOCK_STATE_LOCKED);

    condition->owner = NULL;
    condition->state = AIO4C_COND_STATE_FREE;

    if (current != NULL) {
        current->state = AIO4C_THREAD_STATE_RUNNING;
    }

    return true;
}

void _NotifyCondition(char* file, int line, Condition* condition) {
    Thread* current = ThreadSelf();
    char* name = (current!=NULL)?current->name:NULL;
    dthread("%s:%d: %s NOTIFY %s ON %p\n", file, line, name, (condition->owner!=NULL)?condition->owner->name:NULL, (void*)condition);

#ifndef AIO4C_WIN32
    ErrorCode code = AIO4C_ERROR_CODE_INITIALIZER;
    if ((code.error = pthread_cond_signal(&condition->condition)) != 0) {
        code.condition = condition;
        Raise(AIO4C_LOG_LEVEL_WARN, AIO4C_THREAD_CONDITION_ERROR_TYPE, AIO4C_THREAD_CONDITION_NOTIFY_ERROR, &code);
    }
#else /* AIO4C_WIN32 */
#ifdef AIO4C_HAVE_CONDITION
    WakeConditionVariable(&condition->condition);
#else /* AIO4C_HAVE_CONDITION */
    ErrorCode code = AIO4C_ERROR_CODE_INITIALIZER;

    dthread("[NOTIFY CONDITION %p] %s about to wait on mutex\n", (void*)condition, name);

    switch (WaitForSingleObject(condition->mutex, INFINITE)) {
        case WAIT_OBJECT_0:
            dthread("[NOTIFY CONDITION %p] %s gained mutex\n", (void*)condition, name);
            break;
        case WAIT_ABANDONED:
            dthread("[NOTIFY CONDITION %p] %s abandoned mutex\n", (void*)condition, name);
            Log(AIO4C_LOG_LEVEL_WARN, "%s:%d: WaitForSingleObject: abandon", __FILE__, __LINE__);
            return;
        case WAIT_FAILED:
            dthread("[NOTIFY CONDITION %p] %s failed mutex with code 0x%08lx\n", (void*)condition, name, GetLastError());
            code.source = AIO4C_ERRNO_SOURCE_SYS;
            code.condition = condition;
            Raise(AIO4C_LOG_LEVEL_WARN, AIO4C_THREAD_CONDITION_ERROR_TYPE, AIO4C_THREAD_CONDITION_NOTIFY_ERROR, &code);
            return;
        default:
            break;
    }

    dthread("[NOTIFY CONDITION %p] %s setting event\n", (void*)condition, name);

    if (SetEvent(condition->event) == 0) {
        code.source = AIO4C_ERRNO_SOURCE_SYS;
        code.condition = condition;
        Raise(AIO4C_LOG_LEVEL_WARN, AIO4C_THREAD_CONDITION_ERROR_TYPE, AIO4C_THREAD_CONDITION_NOTIFY_ERROR, &code);
    }

    dthread("[NOTIFY CONDITION %p] %s releasing mutex\n", (void*)condition, name);
    if (ReleaseMutex(condition->mutex) == 0) {
        dthread("[NOTIFY CONDITION %p] %s failed releasing mutex with code 0x%08lx\n", (void*)condition, name, GetLastError());
        code.source = AIO4C_ERRNO_SOURCE_SYS;
        code.condition = condition;
        Raise(AIO4C_LOG_LEVEL_WARN, AIO4C_THREAD_CONDITION_ERROR_TYPE, AIO4C_THREAD_CONDITION_NOTIFY_ERROR, &code);
    }
    dthread("[NOTIFY CONDITION %p] %s released mutex\n", (void*)condition, name);
#endif /* AIO4C_HAVE_CONDITION */
#endif /* AIO4C_WIN32 */
}

Thread* ConditionGetOwner(Condition* condition) {
    return condition->owner;
}

ConditionState ConditionGetState(Condition* condition) {
    return condition->state;
}

void FreeCondition(Condition** condition) {
    Condition* pCondition = NULL;
#ifndef AIO4C_WIN32
    ErrorCode code = AIO4C_ERROR_CODE_INITIALIZER;
#endif /* AIO4C_WIN32 */

    if (condition != NULL && (pCondition = *condition) != NULL) {
#ifndef AIO4C_WIN32
        if ((code.error = pthread_cond_destroy(&pCondition->condition)) != 0) {
            code.condition = pCondition;
            Raise(AIO4C_LOG_LEVEL_WARN, AIO4C_THREAD_CONDITION_ERROR_TYPE, AIO4C_THREAD_CONDITION_DESTROY_ERROR, &code);
        }
#else /* AIO4C_WIN32 */
#ifndef AIO4C_HAVE_CONDITION
        CloseHandle(pCondition->mutex);
        CloseHandle(pCondition->event);
#endif /* AIO4C_HAVE_CONDITION */
#endif /* AIO4C_WIN32 */

        aio4c_free(pCondition);
        *condition = NULL;
    }
}
