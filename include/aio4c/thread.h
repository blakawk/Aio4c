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
#ifndef __AIO4C_THREAD_H__
#define __AIO4C_THREAD_H__

#include <aio4c/types.h>

#ifndef AIO4C_WIN32
#include <pthread.h>
#else /* AIO4C_WIN32 */
#include <winbase.h>
#endif /* AIO4C_WIN32 */

#include <stdarg.h>
#include <stdio.h>

#define aio4c_thread_run(run) \
    (aio4c_bool_t(*)(void*))run

#define aio4c_thread_init(init) \
    (aio4c_bool_t(*)(void*))init

#define aio4c_thread_exit(exit) \
    (void(*)(void*))exit

#define aio4c_thread_arg(arg) \
    (void*)arg

#ifndef AIO4C_DEBUG_THREADS
#define AIO4C_DEBUG_THREADS 0
#endif

#define dthread(fmt,...) {                 \
    if (AIO4C_DEBUG_THREADS) {             \
        fprintf(stderr, fmt, __VA_ARGS__); \
    }                                      \
}

typedef enum e_ThreadState {
    AIO4C_THREAD_STATE_RUNNING,
    AIO4C_THREAD_STATE_BLOCKED,
    AIO4C_THREAD_STATE_IDLE,
    AIO4C_THREAD_STATE_EXITED,
    AIO4C_THREAD_STATE_JOINED,
    AIO4C_THREAD_STATE_MAX
} ThreadState;

extern char* ThreadStateString[AIO4C_THREAD_STATE_MAX];

#define AIO4C_THREAD_INITIALIZER {    \
    .name = NULL,                     \
    .state = AIO4C_THREAD_STATE_NONE, \
    .running = false,                 \
    .lock = AIO4C_LOCK_INITIALIZER,   \
    .init = NULL,                     \
    .run = NULL,                      \
    .exit = NULL,                     \
    .arg = NULL,                      \
}

#ifndef __AIO4C_THREAD_DEFINED__
#define __AIO4C_THREAD_DEFINED__
typedef struct s_Thread Thread;
#endif /* __AIO4C_THREAD_DEFINED__ */

#ifndef __AIO4C_LOCK_DEFINED__
#define __AIO4C_LOCK_DEFINED__
typedef struct s_Lock Lock;
#endif /* __AIO4C_LOCK_DEFINED__ */

struct s_Thread {
    char*             name;
#ifndef AIO4C_WIN32
    pthread_t         id;
#else /* AIO4C_WIN32 */
    DWORD             id;
    HANDLE            handle;
#endif /* AIO4C_WIN32 */
    ThreadState       state;
    aio4c_bool_t      running;
    Lock*             lock;
    aio4c_bool_t    (*init)(void*);
    aio4c_bool_t    (*run)(void*);
    void            (*exit)(void*);
    void*             arg;
};

extern int GetNumThreads(void);

extern char* BuildThreadName(int suffixLen, char* suffix, ...) __attribute__((format(printf, 2, 3)));

extern void NewThread(char* name, aio4c_bool_t (*init)(void*), aio4c_bool_t (*run)(void*), void (*exit)(void*), void* arg, Thread** pThread);

extern aio4c_bool_t ThreadRunning(Thread* thread);

#ifdef AIO4C_DEBUG
#define ThreadSelf() \
    _ThreadSelf()
#else /* AIO4C_DEBUG */
#define ThreadSelf() \
    NULL
#endif /* AIO4C_DEBUG */
extern Thread* _ThreadSelf(void);

extern Thread* ThreadJoin(Thread* thread);

#endif
