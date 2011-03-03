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
#include <pthread.h>

#else /* AIO4C_WIN32 */

#include <winbase.h>
#include <winsock2.h>

#endif /* AIO4C_WIN32 */

#include <stdio.h>

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
#ifndef AIO4C_WIN32
    pthread_t         id;
#else /* AIO4C_WIN32 */
    DWORD             id;
#endif /* AIO4C_WIN32 */
    ThreadState       state;
    aio4c_bool_t      stateValid;
    Lock*             lock;
    void            (*init)(void*);
    aio4c_bool_t    (*run)(void*);
    void            (*exit)(void*);
    void*             arg;
} Thread;

struct s_Lock {
    LockState       state;
    Thread*         owner;
#ifndef AIO4C_WIN32
    pthread_mutex_t mutex;
#endif /* AIO4C_WIN32 */
};

typedef struct s_Condition {
    Thread*        owner;
#ifndef AIO4C_WIN32
    pthread_cond_t condition;
#endif /* AIO4C_WIN32 */
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

#endif /* __AIO4C_CONNECTION_DEFINED__ */

#ifndef __AIO4C_BUFFER_DEFINED__
#define __AIO4C_BUFFER_DEFINED__

typedef struct s_Buffer Buffer;

#endif /* __AIO4C_BUFFER_DEFINED__ */

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

typedef enum e_SelectionOperation {
#ifndef AIO4C_WIN32
    AIO4C_OP_READ = POLLIN,
    AIO4C_OP_WRITE = POLLOUT
#else /* AIO4C_WIN32 */
    AIO4C_OP_READ = POLLRDNORM,
    AIO4C_OP_WRITE = POLLWRNORM
#endif /* AIO4C_WIN32 */
} SelectionOperation;

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
#endif /* AIO4C_WIN32 */
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
