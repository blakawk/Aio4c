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
#include <aio4c/thread.h>

#include <aio4c/alloc.h>
#include <aio4c/condition.h>
#include <aio4c/error.h>
#include <aio4c/lock.h>
#include <aio4c/log.h>
#include <aio4c/stats.h>
#include <aio4c/types.h>

#ifndef AIO4C_WIN32
#include <errno.h>
#include <pthread.h>
#else /* AIO4C_WIN32 */
#include <winbase.h>
#endif /* AIO4C_WIN32 */

#include <stdarg.h>
#include <string.h>

char* ThreadStateString[AIO4C_THREAD_STATE_MAX] = {
    "NONE",
    "RUNNING",
    "BLOCKED",
    "IDLE",
    "EXITED",
    "JOINED"
};

static Thread*      _threads[AIO4C_MAX_THREADS];
static aio4c_bool_t _threadsInitialized = false;
static int          _numThreads = 0;
static int          _numThreadsRunning = 0;
#ifndef AIO4C_WIN32
static pthread_mutex_t  _threadsLock = PTHREAD_MUTEX_INITIALIZER;
#else /* AIO4C_WIN32 */
static CRITICAL_SECTION _threadsLock;
#endif /* AIO4C_WIN32 */

int GetNumThreads(void) {
    return _numThreadsRunning;
}

static Thread _MainThread;

static void _ThreadCleanup(Thread* thread) {
    int i = 0;

    ReleaseLock(thread->lock);

    if (thread->exit != NULL) {
        thread->exit(thread->arg);
    }

    thread->state = AIO4C_THREAD_STATE_EXITED;

    _numThreadsRunning--;

#ifndef AIO4C_WIN32
    pthread_mutex_lock(&_threadsLock);
#else /* AIO4C_WIN32 */
    EnterCriticalSection(&_threadsLock);
#endif /* AIO4C_WIN32 */
    for (i = 0; i < _numThreads; i++) {
        if (_threads[i] == thread) {
            _threads[i] = NULL;
        }
    }
#ifndef AIO4C_WIN32
    pthread_mutex_unlock(&_threadsLock);
#else /* AIO4C_WIN32 */
    LeaveCriticalSection(&_threadsLock);
#endif /* AIO4C_WIN32 */
}

static Thread* _runThread(Thread* thread) {
    TakeLock(thread->lock);

#ifndef AIO4C_WIN32
    pthread_cleanup_push((void(*)(void*))_ThreadCleanup, (void*)thread);
#else /* AIO4C_WIN32 */
    /* no thread cleanup on Win32 */
#endif /* AIO4C_WIN32 */

    aio4c_bool_t run = true;

    _numThreadsRunning++;

    thread->running = true;

    if (thread->init != NULL) {
        run = thread->init(thread->arg);
    }

    thread->initialized = true;
    thread->running = run;

    NotifyCondition(thread->condInit);

    while (thread->running) {
        ReleaseLock(thread->lock);

        run = thread->run(thread->arg);

        TakeLock(thread->lock);

        if (thread->running) {
            thread->running = run;
        }
    }

#ifndef AIO4C_WIN32
    pthread_cleanup_pop(1);
#else /* AIO4C_WIN32 */
    _ThreadCleanup(thread);
#endif /* AIO4C_WIN32 */

#ifndef AIO4C_WIN32
    pthread_exit(thread);
#else /* AIO4C_WIN32 */
    ExitThread(0);
#endif /* AIO4C_WIN32 */

    return thread;
}

void ThreadMain(void) {
    _threadsInitialized  = true;
    memset(_threads, 0, AIO4C_MAX_THREADS * sizeof(Thread*));
#ifdef AIO4C_WIN32
    InitializeCriticalSection(&_threadsLock);
#endif /* AIO4C_WIN32 */

    _MainThread.state    = AIO4C_THREAD_STATE_RUNNING;
    _MainThread.name     = "aio4c";
#ifndef AIO4C_WIN32
    _MainThread.id       = pthread_self();
#else /* AIO4C_WIN32 */
    _MainThread.id       = GetCurrentThreadId();
#endif /* AIO4C_WIN32 */
    _MainThread.lock     = NULL;
    _MainThread.run      = NULL;
    _MainThread.exit     = NULL;
    _MainThread.init     = NULL;
    _MainThread.arg      = NULL;
    _MainThread.running  = true;

    _threads[0] = &_MainThread;
    _numThreads++;
}

static void _addThread(Thread* thread) {
#ifndef AIO4C_WIN32
    pthread_mutex_lock(&_threadsLock);
#else /* AIO4C_WIN32 */
    EnterCriticalSection(&_threadsLock);
#endif /* AIO4C_WIN32 */

    _threads[_numThreads++] = thread;

#ifndef AIO4C_WIN32
    pthread_mutex_unlock(&_threadsLock);
#else /* AIO4C_WIN32 */
    LeaveCriticalSection(&_threadsLock);
#endif /* AIO4C_WIN32 */
}

Thread* _ThreadSelf(void) {
    int i = 0;
    Thread* self = NULL;

    if (_threadsInitialized == false) {
        return NULL;
    }

#ifndef AIO4C_WIN32
    pthread_mutex_lock(&_threadsLock);
#else /* AIO4C_WIN32 */
    EnterCriticalSection(&_threadsLock);
#endif /* AIO4C_WIN32 */

    for (i = 0; i < _numThreads && i < AIO4C_MAX_THREADS; i++) {
        if (_threads[i] != NULL) {
#ifndef AIO4C_WIN32
            pthread_t tid = pthread_self();
            if (memcmp(&_threads[i]->id, &tid, sizeof(pthread_t)) == 0) {
                self = _threads[i];
                break;
            }
#else /* AIO4C_WIN32 */
            DWORD tid = GetCurrentThreadId();
            if (memcmp(&_threads[i]->id, &tid, sizeof(DWORD)) == 0) {
                self = _threads[i];
                break;
            }
#endif /* AIO4C_WIN32 */
        }
    }

#ifndef AIO4C_WIN32
    pthread_mutex_unlock(&_threadsLock);
#else /* AIO4C_WIN32 */
    LeaveCriticalSection(&_threadsLock);
#endif /* AIO4C_WIN32 */

    return self;
}

aio4c_bool_t ThreadStart(Thread* thread) {
    ErrorCode code = AIO4C_ERROR_CODE_INITIALIZER;

    TakeLock(thread->lock);

#ifndef AIO4C_WIN32
    if ((code.error = pthread_create(&thread->id, NULL, (void*(*)(void*))_runThread, (void*)thread)) != 0) {
        code.thread = thread;
#else /* AIO4C_WIN32 */
    thread->handle = CreateThread(NULL, 0, (LPTHREAD_START_ROUTINE)_runThread, (LPVOID)thread, 0, &thread->id);
    if (thread->handle == NULL) {
        code.source = AIO4C_ERRNO_SOURCE_SYS;
#endif /* AIO4C_WIN32 */
        Raise(AIO4C_LOG_LEVEL_ERROR, AIO4C_THREAD_ERROR_TYPE, AIO4C_THREAD_CREATE_ERROR, &code);
        ReleaseLock(thread->lock);
        return false;
    }

    _addThread(thread);

    while (!thread->initialized) {
        WaitCondition(thread->condInit, thread->lock);
    }

    ReleaseLock(thread->lock);

    return true;
}

Thread* NewThread(char* name, aio4c_bool_t (*init)(void*), aio4c_bool_t (*run)(void*), void (*exit)(void*), void* arg) {
    Thread* thread = NULL;
    ErrorCode code = AIO4C_ERROR_CODE_INITIALIZER;

    if (_numThreads >= AIO4C_MAX_THREADS) {
        Log(AIO4C_LOG_LEVEL_FATAL, "too much threads (%d), cannot create more", _numThreads);
        return NULL;
    }

    if ((thread = aio4c_malloc(sizeof(Thread))) == NULL) {
#ifndef AIO4C_WIN32
        code.error = errno;
#else /* AIO4C_WIN32 */
        code.source = AIO4C_ERRNO_SOURCE_SYS;
#endif /* AIO4C_WIN32 */
        code.size = sizeof(Thread);
        code.type = "Thread";
        Raise(AIO4C_LOG_LEVEL_ERROR, AIO4C_ALLOC_ERROR_TYPE, AIO4C_ALLOC_ERROR, &code);
        return NULL;
    }

    thread->name = name;
    thread->state = AIO4C_THREAD_STATE_NONE;
    thread->lock = NewLock();
    thread->condInit = NewCondition();

    if (thread->lock == NULL || thread->condInit == NULL) {
        aio4c_free(thread);
        return NULL;
    }

    thread->initialized = false;
    thread->init = init;
    thread->run = run;
    thread->exit = exit;
    thread->arg = arg;
    thread->running = false;

    return thread;
}

aio4c_bool_t ThreadRunning(Thread* thread) {
    aio4c_bool_t running = false;

    if (thread != NULL) {
        running = (running || thread->state == AIO4C_THREAD_STATE_RUNNING);
        running = (running || thread->state == AIO4C_THREAD_STATE_IDLE);
        running = (running || thread->state == AIO4C_THREAD_STATE_BLOCKED);
        running = (running || thread->state == AIO4C_THREAD_STATE_EXITED);
        running = (running || thread->running);
    }

    return running;
}

void FreeThread(Thread** thread) {
    Thread* pThread = NULL;

    if (thread != NULL && (pThread = *thread) != NULL) {
        FreeCondition(&pThread->condInit);
        FreeLock(&pThread->lock);
        aio4c_free(pThread);
        *thread = NULL;
    }
}

void ThreadJoin(Thread* thread) {
    ErrorCode code = AIO4C_ERROR_CODE_INITIALIZER;
    void* returnValue = NULL;

    if (ThreadRunning(thread)) {
        _numThreadsRunning--;

#ifndef AIO4C_WIN32
        if ((code.error = pthread_join(thread->id, &returnValue)) != 0) {
            code.thread = thread;
            Raise(AIO4C_LOG_LEVEL_ERROR, AIO4C_THREAD_ERROR_TYPE, AIO4C_THREAD_JOIN_ERROR, &code);
        }
#else /* AIO4C_WIN32 */
        switch (WaitForSingleObject(thread->handle, INFINITE)) {
            case WAIT_OBJECT_0:
                break;
            case WAIT_ABANDONED:
                Log(AIO4C_LOG_LEVEL_ERROR, "%s:%d: join thread %p[%s]: abandon",
                        __FILE__, __LINE__, (void*)thread, ThreadStateString[thread->state]);
                break;
            case WAIT_FAILED:
                code.source = AIO4C_ERRNO_SOURCE_SYS;
                code.thread = thread;
                Raise(AIO4C_LOG_LEVEL_ERROR, AIO4C_THREAD_ERROR_TYPE, AIO4C_THREAD_JOIN_ERROR, &code);
                break;
            default:
                break;
        }

        if (GetExitCodeThread(thread->handle, (LPDWORD)&returnValue) == 0) {
            code.source = AIO4C_ERRNO_SOURCE_SYS;
            code.thread = thread;
            Raise(AIO4C_LOG_LEVEL_ERROR, AIO4C_THREAD_ERROR_TYPE, AIO4C_THREAD_JOIN_ERROR, &code);
        }

        CloseHandle(thread->handle);
#endif /* AIO4C_WIN32 */

        _numThreadsRunning++;

        thread->state = AIO4C_THREAD_STATE_JOINED;
    }

    FreeThread(&thread);
}

