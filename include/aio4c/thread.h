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

#ifndef AIO4C_WIN32
#include <poll.h>
#else
#include <winsock2.h>
#endif

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
#define aio4c_cond_timedwait pthread_cond_timedwait
#define aio4c_cond_signal pthread_cond_signal
#define aio4c_cond_destroy pthread_cond_destroy

#define aio4c_thread_run(run) \
    (aio4c_bool_t(*)(void*))run

#define aio4c_thread_handler(handler) \
    (void(*)(void*))handler

#define aio4c_thread_arg(arg) \
    (void*)arg

#define aio4c_remove_callback(callback) \
    (aio4c_bool_t(*)(QueueItem*,void*))callback

#define aio4c_remove_discriminant(discriminant) \
    (void*)discriminant

#define AIO4C_MAX_THREADS 32

#ifndef AIO4C_DEBUG_LOCKS
#define AIO4C_DEBUG_LOCKS 0
#endif

#define debugLock(fmt, ...) \
    if (AIO4C_DEBUG_LOCKS) { \
        fprintf(stderr, fmt, __VA_ARGS__); \
    }

typedef enum e_ThreadState {
    AIO4C_THREAD_STATE_RUNNING,
    AIO4C_THREAD_STATE_BLOCKED,
    AIO4C_THREAD_STATE_IDLE,
    AIO4C_THREAD_STATE_STOPPED,
    AIO4C_THREAD_STATE_EXITED,
    AIO4C_THREAD_STATE_JOINED,
    AIO4C_THREAD_STATE_MAX
} ThreadState;

typedef enum e_LockState {
    AIO4C_LOCK_STATE_DESTROYED,
    AIO4C_LOCK_STATE_FREE,
    AIO4C_LOCK_STATE_LOCKED,
    AIO4C_LOCK_STATE_MAX
} LockState;

typedef struct s_Lock Lock;

typedef struct s_Thread {
    char*             name;
    pthread_t         id;
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
    AIO4C_QUEUE_ITEM_EVENT,
    AIO4C_QUEUE_ITEM_DATA,
    AIO4C_QUEUE_ITEM_TASK,
    AIO4C_QUEUE_ITEM_EXIT,
    AIO4C_QUEUE_ITEM_MAX
} QueueItemType;

typedef struct s_QueueEventItem {
    Event type;
    void* source;
} QueueEventItem;

#ifndef __AIO4C_CONNECTION_DEFINED__
#define __AIO4C_CONNECTION_DEFINED__

typedef struct s_Connection Connection;

#endif

#ifndef __AIO4C_BUFFER_DEFINED__
#define __AIO4C_BUFFER_DEFINED__

typedef struct s_Buffer Buffer;

#endif

typedef struct s_QueueTaskItem {
    Event       event;
    Connection* connection;
    Buffer*     buffer;
} QueueTaskItem;

typedef union u_QueueItemData {
    QueueTaskItem task;
    QueueEventItem event;
    void*  data;
} QueueItemData;

typedef struct s_QueueItem {
    QueueItemType type;
    QueueItemData content;
} QueueItem;

typedef struct s_Queue {
    QueueItem**  items;
    int          numItems;
    int          maxItems;
    Condition*   condition;
    Lock*        lock;
    aio4c_bool_t valid;
    aio4c_bool_t exit;
    aio4c_bool_t emptied;
} Queue;

#ifndef AIO4C_WIN32
#define AIO4C_OP_READ POLLIN
#define AIO4C_OP_WRITE POLLOUT
#else
#define AIO4C_OP_READ POLLRDNORM
#define AIO4C_OP_WRITE POLLWRNORM
#endif

typedef int SelectionOperation;

typedef struct s_SelectionKey {
    SelectionOperation operation;
    int                index;
    aio4c_socket_t     fd;
    void*              attachment;
    int                poll;
    int                count;
    short              result;
} SelectionKey;

typedef struct s_Selector {
    aio4c_pipe_t   pipe;
    SelectionKey** keys;
    int            numKeys;
    int            maxKeys;
    int            curKey;
    int            curKeyCount;
    aio4c_poll_t*  polls;
    int            numPolls;
    int            maxPolls;
    Lock*          lock;
#ifdef AIO4C_WIN32
    int            port;
#endif
} Selector;

extern int GetNumThreads(void);

extern Lock* NewLock(void);

extern Lock* TakeLock(Lock* lock);

extern Lock* ReleaseLock(Lock* lock);

extern void FreeLock(Lock** lock);

extern Condition* NewCondition(void);

extern aio4c_bool_t WaitCondition(Condition* condition, Lock* lock);

extern void NotifyCondition(Condition* condition);

extern void FreeCondition(Condition** condition);

extern Queue* NewQueue(void);

extern aio4c_bool_t EnqueueDataItem(Queue* queue, void* data);

extern aio4c_bool_t EnqueueExitItem(Queue* queue);

extern aio4c_bool_t EnqueueEventItem(Queue* queue, Event type, void* source);

extern aio4c_bool_t EnqueueTaskItem(Queue* queue, Event event, Connection* connection, Buffer* buffer);

extern aio4c_bool_t Dequeue(Queue* queue, QueueItem* item, aio4c_bool_t wait);

extern aio4c_bool_t RemoveAll(Queue* queue, aio4c_bool_t (*removeCallback)(QueueItem*,void*), void* discriminant);

extern void FreeQueue(Queue** queue);

extern Selector* NewSelector(void);

extern SelectionKey* Register(Selector* selector, SelectionOperation operation, aio4c_socket_t fd, void* attachment);

extern void Unregister(Selector* selector, SelectionKey* key, aio4c_bool_t unregisterAll);

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
