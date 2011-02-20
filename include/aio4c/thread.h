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

#include <aio4c/event.h>
#include <aio4c/types.h>

#include <poll.h>
#include <pthread.h>
#include <stdio.h>

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

#define AIO4C_MAX_THREADS 32

#ifndef AIO4C_DEBUG_LOCKS
#define AIO4C_DEBUG_LOCKS 0
#endif

#define debugLock(fmt, ...) \
    if (AIO4C_DEBUG_LOCKS) { \
        fprintf(stderr, fmt, __VA_ARGS__); \
    }

typedef enum e_ThreadState {
    RUNNING,
    BLOCKED,
    IDLE,
    STOPPED,
    EXITED,
    JOINED
} ThreadState;

typedef enum e_LockState {
    DESTROYED,
    FREE,
    LOCKED
} LockState;

typedef struct s_Lock Lock;

typedef struct s_Thread {
    char*             name;
    aio4c_thread_t    id;
    ThreadState       state;
    aio4c_bool_t      stateValid;
    Lock*             lock;
    void            (*init)(void*);
    aio4c_bool_t    (*run)(void*);
    void            (*exit)(void*);
    void*             arg;
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

typedef enum e_QueueItemType {
    EVENT,
    DATA,
    EXIT
} QueueItemType;

typedef struct s_QueueEventItem {
    Event type;
    void* source;
} QueueEventItem;

typedef union u_QueueItemData {
    QueueEventItem event;
    void*  data;
} QueueItemData;

typedef struct s_QueueItem {
    QueueItemType type;
    QueueItemData content;
} QueueItem;

typedef struct s_Queue {
    QueueItem**  items;
    aio4c_size_t numItems;
    aio4c_size_t maxItems;
    Condition*   condition;
    Lock*        lock;
    aio4c_bool_t valid;
    aio4c_bool_t emptied;
} Queue;

typedef enum e_SelectionOperation {
    AIO4C_OP_READ = POLLIN,
    AIO4C_OP_WRITE = POLLOUT
} SelectionOperation;

typedef struct s_SelectionKey {
    SelectionOperation operation;
    aio4c_size_t       index;
    aio4c_socket_t     fd;
    void*              attachment;
    aio4c_size_t       poll;
    short              result;
} SelectionKey;

typedef struct s_Selector {
    aio4c_pipe_t  pipe;
    SelectionKey* keys;
    aio4c_size_t  numKeys;
    aio4c_size_t  maxKeys;
    int           curKey;
    aio4c_poll_t* polls;
    aio4c_size_t  numPolls;
    aio4c_size_t  maxPolls;
    Lock*         lock;
} Selector;

extern Lock* NewLock(void);

extern Lock* TakeLock(Lock* lock);

extern Lock* ReleaseLock(Lock* lock);

extern void FreeLock(Lock** lock);

extern Condition* NewCondition(void);

extern aio4c_bool_t WaitCondition(Condition* condition, Lock* lock);

extern void NotifyCondition(Condition* condition, Lock* lock);

extern void FreeCondition(Condition** condition);

extern Queue* NewQueue(void);

extern QueueItem* NewDataItem(void* data);

extern QueueItem* NewEventItem(Event event, void* source);

extern QueueItem* NewExitItem(void);

extern void FreeItem(QueueItem** item);

extern aio4c_bool_t Dequeue(Queue* queue, QueueItem** item, aio4c_bool_t wait);

extern aio4c_bool_t Enqueue(Queue* queue, QueueItem* item);

extern void FreeQueue(Queue** queue);

extern Selector* NewSelector(void);

extern SelectionKey* Register(Selector* selector, SelectionOperation operation, aio4c_socket_t fd, void* attachment);

extern void Unregister(Selector* selector, SelectionKey* key);

extern aio4c_size_t Select(Selector* selector);

extern void SelectorWakeUp(Selector* selector);

extern aio4c_bool_t SelectionKeyReady(Selector* selector, SelectionKey** key);

extern void FreeSelector(Selector** selector);

extern Thread* ThreadMain(char* name);

extern Thread* NewThread(char* name, void (*init)(void*), aio4c_bool_t (*run)(void*), void (*exit)(void*), void* arg);

extern aio4c_bool_t ThreadRunning(Thread* thread);

extern Thread* ThreadCancel(Thread* thread, aio4c_bool_t force);

extern Thread* ThreadSelf(void);

extern Thread* ThreadJoin(Thread* thread);

extern void FreeThread(Thread** thread);

#endif
