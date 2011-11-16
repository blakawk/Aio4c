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
#include <aio4c/lock.h>

#include <aio4c/alloc.h>
#include <aio4c/error.h>
#include <aio4c/stats.h>
#include <aio4c/thread.h>
#include <aio4c/types.h>

#ifndef AIO4C_WIN32
#include <errno.h>
#include <pthread.h>
#else /* AIO4C_WIN32 */
#include <winbase.h>
#endif /* AIO4C_WIN32 */

struct s_Lock {
    LockState        state;
    Thread*          owner;
#ifndef AIO4C_WIN32
    pthread_mutex_t  mutex;
#else /* AIO4C_WIN32 */
    CRITICAL_SECTION mutex;
#endif /* AIO4C_WIN32 */
};

char* LockStateString[AIO4C_LOCK_STATE_MAX] = {
    "DESTROYED",
    "FREE",
    "LOCKED"
};

Lock* NewLock(void) {
    Lock* pLock = NULL;
    ErrorCode code = AIO4C_ERROR_CODE_INITIALIZER;

    if ((pLock = aio4c_malloc(sizeof(Lock))) == NULL) {
#ifndef AIO4C_WIN32
        code.error = errno;
#else /* AIO4C_WIN32 */
        code.source = AIO4C_ERRNO_SOURCE_SYS;
#endif /* AIO4C_WIN32 */
        code.size = sizeof(Lock);
        code.type = "Lock";
        Raise(AIO4C_LOG_LEVEL_ERROR, AIO4C_ALLOC_ERROR_TYPE, AIO4C_ALLOC_ERROR, &code);
        return NULL;
    }

    pLock->state = AIO4C_LOCK_STATE_DESTROYED;
    pLock->owner = NULL;

#ifndef AIO4C_WIN32
    if ((code.error = pthread_mutex_init(&pLock->mutex, NULL)) != 0) {
        code.lock = pLock;
        Raise(AIO4C_LOG_LEVEL_ERROR, AIO4C_THREAD_LOCK_ERROR_TYPE, AIO4C_THREAD_LOCK_INIT_ERROR, &code);
        aio4c_free(pLock);
        return NULL;
    }
#else /* AIO4C_WIN32 */
    InitializeCriticalSection(&pLock->mutex);
#endif /* AIO4C_WIN32 */

    pLock->state = AIO4C_LOCK_STATE_FREE;

    return pLock;
}

Lock* _TakeLock(char* file, int line, Lock* lock) {
#ifndef AIO4C_WIN32
    ErrorCode code = AIO4C_ERROR_CODE_INITIALIZER;
#endif /* AIO4C_WIN32 */
    Thread* current = ThreadSelf();

    ProbeTimeStart(AIO4C_TIME_PROBE_BLOCK);

    if (current != NULL) {
        ThreadSetState(current, AIO4C_THREAD_STATE_BLOCKED);
    }

    dthread("%s:%d: %s lock %p\n", file, line, (current!=NULL)?ThreadGetName(current):NULL, (void*)lock);

#ifndef AIO4C_WIN32
    if ((code.error = pthread_mutex_lock(&lock->mutex)) != 0) {
        code.lock = lock;

        if (current != NULL) {
            ThreadSetState(current, AIO4C_THREAD_STATE_RUNNING);
        }

        Raise(AIO4C_LOG_LEVEL_ERROR, AIO4C_THREAD_LOCK_ERROR_TYPE, AIO4C_THREAD_LOCK_TAKE_ERROR, &code);
        return NULL;
    }
#else /* AIO4C_WIN32 */
    EnterCriticalSection(&lock->mutex);
#endif /* AIO4C_WIN32 */

    lock->owner = current;
    lock->state = AIO4C_LOCK_STATE_LOCKED;

    if (current != NULL) {
        ThreadSetState(current, AIO4C_THREAD_STATE_RUNNING);
    }

    ProbeTimeEnd(AIO4C_TIME_PROBE_BLOCK);

    return lock;
}

Lock* _ReleaseLock(char* file, int line, Lock* lock) {
    Thread* current = ThreadSelf();
#ifndef AIO4C_WIN32
    ErrorCode code = AIO4C_ERROR_CODE_INITIALIZER;
#endif /* AIO4C_WIN32 */

    dthread("%s:%d: %s unlock %p\n", file, line, (current!=NULL)?ThreadGetName(current):NULL, (void*)lock);

    lock->state = AIO4C_LOCK_STATE_FREE;
    lock->owner = NULL;

#ifndef AIO4C_WIN32
    if ((code.error = pthread_mutex_unlock(&lock->mutex)) != 0) {
        code.lock = lock;
        Raise(AIO4C_LOG_LEVEL_WARN, AIO4C_THREAD_LOCK_ERROR_TYPE, AIO4C_THREAD_LOCK_RELEASE_ERROR, &code);
        return NULL;
    }
#else /* AIO4C_WIN32 */
    LeaveCriticalSection(&lock->mutex);
#endif /* AIO4C_WIN32 */

    return lock;
}

Lock* LockSetOwner(Lock* lock, Thread* owner) {
    lock->owner = owner;
    return lock;
}

Thread* LockGetOwner(Lock* lock) {
    return lock->owner;
}

Lock* LockSetState(Lock* lock, LockState state) {
    lock->state = state;
    return lock;
}

LockState LockGetState(Lock* lock) {
    return lock->state;
}

void* LockGetMutex(Lock* lock) {
    return &lock->mutex;
}

void FreeLock(Lock** lock) {
    Lock * pLock = NULL;
#ifndef AIO4C_WIN32
    ErrorCode code = AIO4C_ERROR_CODE_INITIALIZER;
#endif /* AIO4C_WIN32 */

    if (lock != NULL && (pLock = *lock) != NULL) {
        pLock->state = AIO4C_LOCK_STATE_DESTROYED;

#ifndef AIO4C_WIN32
        if ((code.error = pthread_mutex_destroy(&pLock->mutex)) != 0) {
            code.lock = pLock;
            Raise(AIO4C_LOG_LEVEL_WARN, AIO4C_THREAD_LOCK_ERROR_TYPE, AIO4C_THREAD_LOCK_DESTROY_ERROR, &code);
        }
#else /* AIO4C_WIN32 */
        DeleteCriticalSection(&pLock->mutex);
#endif /* AIO4C_WIN32 */

        aio4c_free(pLock);
        *lock = NULL;
    }
}
