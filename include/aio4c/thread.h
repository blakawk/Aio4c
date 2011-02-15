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

#include <pthread.h>

#define aio4c_mutex_init pthread_mutex_init
#define aio4c_mutex_lock pthread_mutex_lock
#define aio4c_mutex_unlock pthread_mutex_unlock
#define aio4c_mutex_destroy pthread_mutex_destroy

#define aio4c_thread_create pthread_create
#define aio4c_thread_self pthread_self
#define aio4c_thread_cancel pthread_cancel
#define aio4c_thread_join pthread_join
#define aio4c_thread_cleanup_push pthread_cleanup_push
#define aio4c_thread_cleanup_pop pthread_cleanup_pop
#define aio4c_thread_exit pthread_exit
#define aio4c_thread_detach pthread_detach
#define aio4c_thread_kill pthread_kill

#define aio4c_cond_init pthread_cond_init
#define aio4c_cond_wait pthread_cond_wait
#define aio4c_cond_signal pthread_cond_signal
#define aio4c_cond_destroy pthread_cond_destroy

#define aio4c_thread_run(run) \
    (aio4c_bool_t(*)(void*))run

#define aio4c_thread_handler(handler) \
    (void(*)(void*))handler

#define aio4c_thread_arg(arg) \
    (void*)arg

typedef enum e_ThreadState {
    RUNNING,
    IDLE,
    BLOCKED,
    STOPPED,
    EXITED,
    JOINED
} ThreadState;

typedef enum e_LockState {
    FREE,
    LOCKED
} LockState;

typedef struct s_Lock Lock;

typedef struct s_Thread {
    char*             name;
    aio4c_thread_t    id;
    ThreadState       state;
    Lock*             lock;
    void            (*init)(void*);
    aio4c_bool_t    (*run)(void*);
    void            (*exit)(void*);
    void*             arg;
    aio4c_size_t      childrenCount;
    aio4c_size_t      maxChildren;
    struct s_Thread** children;
    struct s_Thread*  parent;
} Thread;

struct s_Lock {
    LockState    state;
    Thread*      owner;
    aio4c_lock_t mutex;
};

typedef struct s_Condition {
    Thread*       owner;
    aio4c_cond_t  condition;
} Condition;

extern Lock* NewLock(void);

extern Lock* TakeLock(Thread* owner, Lock* lock);

extern Lock* ReleaseLock(Lock* lock);

extern void FreeLock(Lock** lock);

extern Condition* NewCondition(void);

extern aio4c_bool_t WaitCondition(Thread* owner, Condition* condition, Lock* lock);

extern void NotifyCondition(Thread* notifier, Condition* condition, Lock* lock);

extern void FreeCondition(Condition** condition);

extern Thread* ThreadMain(char* name);

extern Thread* NewThread(Thread* parent, char* name, void (*init)(void*), aio4c_bool_t (*run)(void*), void (*exit)(void*), void* arg);

extern Thread* ThreadMain(char* name);

extern Thread* ThreadCancel(Thread* parent, Thread* thread, aio4c_bool_t force);

extern Thread* ThreadJoin(Thread* parent, Thread* thread);

extern void FreeThread(Thread* parent, Thread** thread);

#endif
