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

#ifndef AIO4C_DEBUG_THREADS
#define AIO4C_DEBUG_THREADS 0
#endif

#define dthread(fmt,...) {                 \
    if (AIO4C_DEBUG_THREADS) {             \
        fprintf(stderr, fmt, __VA_ARGS__); \
    }                                      \
}

#define AIO4C_MAX_THREADS 32

typedef enum e_ThreadState {
    AIO4C_THREAD_STATE_RUNNING,
    AIO4C_THREAD_STATE_BLOCKED,
    AIO4C_THREAD_STATE_IDLE,
    AIO4C_THREAD_STATE_EXITED,
    AIO4C_THREAD_STATE_JOINED,
    AIO4C_THREAD_STATE_MAX
} ThreadState;

extern char* ThreadStateString[AIO4C_THREAD_STATE_MAX];

typedef enum e_LockState {
    AIO4C_LOCK_STATE_DESTROYED,
    AIO4C_LOCK_STATE_FREE,
    AIO4C_LOCK_STATE_LOCKED,
    AIO4C_LOCK_STATE_MAX
} LockState;

extern char* LockStateString[AIO4C_LOCK_STATE_MAX];

typedef enum e_ConditionState {
    AIO4C_COND_STATE_DESTROYED,
    AIO4C_COND_STATE_FREE,
    AIO4C_COND_STATE_WAITED,
    AIO4C_COND_STATE_MAX
} ConditionState;

extern char* ConditionStateString[AIO4C_COND_STATE_MAX];

typedef struct s_Lock Lock;

typedef struct s_Thread {
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
    void            (*init)(void*);
    aio4c_bool_t    (*run)(void*);
    void            (*exit)(void*);
    void*             arg;
} Thread;

struct s_Lock {
    LockState        state;
    Thread*          owner;
#ifndef AIO4C_WIN32
    pthread_mutex_t  mutex;
#else /* AIO4C_WIN32 */
    CRITICAL_SECTION mutex;
#endif /* AIO4C_WIN32 */
};

typedef struct s_Condition {
    ConditionState state;
    Thread*        owner;
#ifndef AIO4C_WIN32
    pthread_cond_t     condition;
#else /* AIO4C_WIN32 */
#if WINVER >= 0x0600
    CONDITION_VARIABLE condition;
#else /* WINVER */
    HANDLE             mutex;
    HANDLE             event;
#endif /* WINVER */
#endif /* AIO4C_WIN32 */
} Condition;

typedef enum e_QueueItemType {
    AIO4C_QUEUE_ITEM_EVENT,
    AIO4C_QUEUE_ITEM_DATA,
    AIO4C_QUEUE_ITEM_TASK,
    AIO4C_QUEUE_ITEM_EXIT,
    AIO4C_QUEUE_ITEM_MAX
} QueueItemType;

extern char* QueueItemTypeString[AIO4C_QUEUE_ITEM_MAX];

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
#ifdef AIO4C_HAVE_POLL
#ifndef AIO4C_WIN32
    AIO4C_OP_READ = POLLIN,
    AIO4C_OP_WRITE = POLLOUT
#else /* AIO4C_WIN32 */
    AIO4C_OP_READ = POLLRDNORM,
    AIO4C_OP_WRITE = POLLWRNORM
#endif /* AIO4C_WIN32 */
#else /* AIO4C_HAVE_POLL */
    AIO4C_OP_READ  = 0x01,
    AIO4C_OP_WRITE = 0x02,
    AIO4C_OP_ERROR = 0x04
#endif /* AIO4C_HAVE_POLL */
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

#ifndef AIO4C_HAVE_POLL
typedef struct s_Poll {
    aio4c_socket_t fd;
    short          events;
    short          revents;
} Poll;
#endif /* AIO4C_HAVE_POLL */

typedef struct s_Selector {
    aio4c_pipe_t    pipe;
    SelectionKey**  keys;
    int             numKeys;
    int             maxKeys;
    int             curKey;
    int             curKeyCount;
#ifdef AIO4C_HAVE_POLL
    aio4c_poll_t*   polls;
#else /* AIO4C_HAVE_POLL */
    Poll*           polls;
#endif /* AIO4C_HAVE_POLL */
    int             numPolls;
    int             maxPolls;
    Lock*           lock;
#ifndef AIO4C_HAVE_PIPE
    int             port;
#endif /* AIO4C_HAVE_PIPE */
} Selector;

extern int GetNumThreads(void);

extern Lock* NewLock(void);

#define TakeLock(lock) \
    _TakeLock(__FILE__, __LINE__, lock)
extern Lock* _TakeLock(char* file, int line, Lock* lock);

#define ReleaseLock(lock) \
    _ReleaseLock(__FILE__, __LINE__, lock)
extern Lock* _ReleaseLock(char* file, int line, Lock* lock);

extern void FreeLock(Lock** lock);

extern Condition* NewCondition(void);

#define WaitCondition(condition,lock) \
    _WaitCondition(__FILE__, __LINE__, condition, lock)
extern aio4c_bool_t _WaitCondition(char* file, int line, Condition* condition, Lock* lock);

#define NotifyCondition(condition) \
    _NotifyCondition(__FILE__, __LINE__, condition)
extern void _NotifyCondition(char* file, int line, Condition* condition);

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

#define Select(selector) \
    _Select(__FILE__, __LINE__, selector)
extern aio4c_size_t _Select(char* file, int line, Selector* selector);

#define SelectorWakeUp(selector) \
    _SelectorWakeUp(__FILE__, __LINE__, selector)
extern void _SelectorWakeUp(char* file, int line, Selector* selector);

extern aio4c_bool_t SelectionKeyReady(Selector* selector, SelectionKey** key);

extern void FreeSelector(Selector** selector);

extern Thread* ThreadMain(char* name);

extern Thread* NewThread(char* name, void (*init)(void*), aio4c_bool_t (*run)(void*), void (*exit)(void*), void* arg);

extern aio4c_bool_t ThreadRunning(Thread* thread);

extern Thread* ThreadSelf(void);

extern Thread* ThreadJoin(Thread* thread);

extern void FreeThread(Thread** thread);

#endif
