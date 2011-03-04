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
#include <aio4c/thread.h>

#include <aio4c/alloc.h>
#include <aio4c/error.h>
#include <aio4c/log.h>
#include <aio4c/stats.h>
#include <aio4c/types.h>

#ifndef AIO4C_WIN32

#include <errno.h>
#include <fcntl.h>
#include <poll.h>
#include <pthread.h>
#include <unistd.h>

#else /* AIO4C_WIN32 */

#include <winbase.h>
#include <winsock2.h>

#endif /* AIO4C_WIN32 */

#include <stdlib.h>
#include <string.h>

char* ThreadStateString[AIO4C_THREAD_STATE_MAX] = {
    "RUNNING",
    "BLOCKED",
    "IDLE",
    "EXITED",
    "JOINED"
};

char* LockStateString[AIO4C_LOCK_STATE_MAX] = {
    "DESTROYED",
    "FREE",
    "LOCKED"
};

char* ConditionStateString[AIO4C_COND_STATE_MAX] = {
    "DESTROYED",
    "FREE",
    "WAITED"
};

static Thread*      _threads[AIO4C_MAX_THREADS];
static aio4c_bool_t _threadsInitialized = false;
static int          _numThreads = 0;
static int          _numThreadsRunning = 0;

int GetNumThreads(void) {
    return _numThreadsRunning;
}

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
#error "pthread_mutex_init not implemented for win32"
#endif /* AIO4C_WIN32 */

    pLock->state = AIO4C_LOCK_STATE_FREE;

    return pLock;
}

Lock* TakeLock(Lock* lock) {
    ErrorCode code = AIO4C_ERROR_CODE_INITIALIZER;
    Thread* current = ThreadSelf();

    ProbeTimeStart(AIO4C_TIME_PROBE_BLOCK);

    if (current != NULL) {
        current->state = AIO4C_THREAD_STATE_BLOCKED;
    }

#ifndef AIO4C_WIN32
    if ((code.error = pthread_mutex_lock(&lock->mutex)) != 0) {
        code.lock = lock;

        if (current != NULL) {
            current->state = AIO4C_THREAD_STATE_RUNNING;
        }

        Raise(AIO4C_LOG_LEVEL_ERROR, AIO4C_THREAD_LOCK_ERROR_TYPE, AIO4C_THREAD_LOCK_TAKE_ERROR, &code);
        return NULL;
    }
#else /* AIO4C_WIN32 */
#error "pthread_mutex_lock not implemented for win32"
#endif /* AIO4C_WIN32 */

    lock->owner = current;
    lock->state = AIO4C_LOCK_STATE_LOCKED;

    if (current != NULL) {
        current->state = AIO4C_THREAD_STATE_RUNNING;
    }

    ProbeTimeEnd(AIO4C_TIME_PROBE_BLOCK);

    return lock;
}

Lock* ReleaseLock(Lock* lock) {
    ErrorCode code = AIO4C_ERROR_CODE_INITIALIZER;

    lock->state = AIO4C_LOCK_STATE_FREE;
    lock->owner = NULL;

#ifndef AIO4C_WIN32
    if ((code.error = pthread_mutex_unlock(&lock->mutex)) != 0) {
        code.lock = lock;
        Raise(AIO4C_LOG_LEVEL_WARN, AIO4C_THREAD_LOCK_ERROR_TYPE, AIO4C_THREAD_LOCK_RELEASE_ERROR, &code);
        return NULL;
    }
#else /* AIO4C_WIN32 */
#error "pthread_mutex_unlock not implemented for win32"
#endif /* AIO4C_WIN32 */
    return lock;
}

void FreeLock(Lock** lock) {
    Lock * pLock = NULL;
    ErrorCode code = AIO4C_ERROR_CODE_INITIALIZER;

    if (lock != NULL && (pLock = *lock) != NULL) {
        pLock->state = AIO4C_LOCK_STATE_DESTROYED;

#ifndef AIO4C_WIN32
        if ((code.error = pthread_mutex_destroy(&pLock->mutex)) != 0) {
            code.lock = pLock;
            Raise(AIO4C_LOG_LEVEL_WARN, AIO4C_THREAD_LOCK_ERROR_TYPE, AIO4C_THREAD_LOCK_DESTROY_ERROR, &code);
        }
#else /* AIO4C_WIN32 */
#error "pthread_mutex_destroy not implemented for win32"
#endif /* AIO4C_WIN32 */

        aio4c_free(pLock);
        *lock = NULL;
    }
}

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
#error "pthread_cond_init not implemented for win32"
#endif /* AIO4C_WIN32 */

    condition->state = AIO4C_COND_STATE_FREE;

    return condition;
}

aio4c_bool_t WaitCondition(Condition* condition, Lock* lock) {
    ErrorCode code = AIO4C_ERROR_CODE_INITIALIZER;
    Thread* current = ThreadSelf();

    condition->owner = current;
    condition->state = AIO4C_COND_STATE_WAITED;

    if (current != NULL) {
        current->state = AIO4C_THREAD_STATE_IDLE;
    }

#ifndef AIO4C_WIN32
    if ((code.error = pthread_cond_wait(&condition->condition, &lock->mutex)) != 0) {
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
#error "pthread_cond_wait not implemented for win32"
#endif /* AIO4C_WIN32 */

    condition->owner = NULL;
    condition->state = AIO4C_COND_STATE_FREE;

    if (current != NULL) {
        current->state = AIO4C_THREAD_STATE_RUNNING;
    }

    return true;
}

void NotifyCondition(Condition* condition) {
    ErrorCode code = AIO4C_ERROR_CODE_INITIALIZER;

#ifndef AIO4C_WIN32
    if ((code.error = pthread_cond_signal(&condition->condition)) != 0) {
        code.condition = condition;
        Raise(AIO4C_LOG_LEVEL_WARN, AIO4C_THREAD_CONDITION_ERROR_TYPE, AIO4C_THREAD_CONDITION_NOTIFY_ERROR, &code);
    }
#else /* AIO4C_WIN32 */
#error "pthread_cond_signal not implemented for win32"
#endif /* AIO4C_WIN32 */
}

void FreeCondition(Condition** condition) {
    Condition* pCondition = NULL;
    ErrorCode code = AIO4C_ERROR_CODE_INITIALIZER;

    if (condition != NULL && (pCondition = *condition) != NULL) {
#ifndef AIO4C_WIN32
        if ((code.error = pthread_cond_destroy(&pCondition->condition)) != 0) {
            code.condition = pCondition;
            Raise(AIO4C_LOG_LEVEL_WARN, AIO4C_THREAD_CONDITION_ERROR_TYPE, AIO4C_THREAD_CONDITION_DESTROY_ERROR, &code);
        }
#else /* AIO4C_WIN32 */
#error "pthread_cond_destroy not implemented for win32"
#endif /* AIO4C_WIN32 */

        aio4c_free(pCondition);
        *condition = NULL;
    }
}

Queue* NewQueue(void) {
    Queue* queue = NULL;
    ErrorCode code = AIO4C_ERROR_CODE_INITIALIZER;

    if ((queue = aio4c_malloc(sizeof(Queue))) == NULL) {
#ifndef AIO4C_WIN32
        code.error = errno;
#else /* AIO4C_WIN32 */
        code.source = AIO4C_ERRNO_SOURCE_SYS;
#endif /* AIO4C_WIN32 */
        code.size = sizeof(Queue);
        code.type = "Queue";
        Raise(AIO4C_LOG_LEVEL_ERROR, AIO4C_ALLOC_ERROR_TYPE, AIO4C_ALLOC_ERROR, &code);
        return NULL;
    }

    queue->items     = NULL;
    queue->numItems  = 0;
    queue->maxItems  = 0;
    queue->lock      = NewLock();
    queue->condition = NewCondition();
    queue->valid     = true;
    queue->emptied   = false;
    queue->exit      = false;

    return queue;
}

aio4c_bool_t Dequeue(Queue* queue, QueueItem* item, aio4c_bool_t wait) {
    int i = 0;
    QueueItem* _item = NULL;
    aio4c_bool_t dequeued = false;

    TakeLock(queue->lock);

    if (queue->emptied) {
        queue->emptied = false;
        ReleaseLock(queue->lock);
        return false;
    }


    while (!queue->exit && wait && queue->numItems == 0 && queue->valid) {
        ProbeTimeStart(AIO4C_TIME_PROBE_IDLE);

        if (!WaitCondition(queue->condition, queue->lock)) {
            break;
        }

        ProbeTimeEnd(AIO4C_TIME_PROBE_IDLE);
    }

    if (queue->numItems > 0) {
        _item = queue->items[0];

        memcpy(item, queue->items[0], sizeof(QueueItem));

        for (i = 0; i < queue->numItems - 1; i++) {
            queue->items[i] = queue->items[i + 1];
        }

        memset(_item, 0, sizeof(QueueItem));

        queue->items[i] = _item;
        queue->numItems--;
        queue->emptied = (queue->numItems == 0);
        dequeued = true;
    } else {
        memset(item, 0, sizeof(QueueItem));
    }

    ReleaseLock(queue->lock);

    return dequeued;
}

static QueueItem* _NewItem(void) {
    QueueItem* item = NULL;
    ErrorCode code = AIO4C_ERROR_CODE_INITIALIZER;

    if ((item = aio4c_malloc(sizeof(QueueItem))) == NULL) {
#ifndef AIO4C_WIN32
        code.error = errno;
#else /* AIO4C_WIN32 */
        code.source = AIO4C_ERRNO_SOURCE_SYS;
#endif /* AIO4C_WIN32 */
        code.size = sizeof(QueueItem);
        code.type = "QueueItem";
        Raise(AIO4C_LOG_LEVEL_ERROR, AIO4C_ALLOC_ERROR_TYPE, AIO4C_ALLOC_ERROR, &code);
        return NULL;
    }

    memset(item, 0, sizeof(QueueItem));

    return item;
}

aio4c_bool_t _Enqueue(Queue* queue, QueueItem* item) {
    QueueItem** items = NULL;
    ErrorCode code = AIO4C_ERROR_CODE_INITIALIZER;

    TakeLock(queue->lock);

    if (queue->exit) {
        ReleaseLock(queue->lock);
        return false;
    }

    if (queue->numItems == queue->maxItems) {
        if ((items = aio4c_realloc(queue->items, (queue->maxItems + 1) * sizeof(QueueItem*))) == NULL) {
#ifndef AIO4C_WIN32
            code.error = errno;
#else /* AIO4C_WIN32 */
            code.source = AIO4C_ERRNO_SOURCE_SYS;
#endif /* AIO4C_WIN32 */
            code.size = (queue->maxItems + 1) * sizeof(QueueItem*);
            code.type = "QueueItem*";
            Raise(AIO4C_LOG_LEVEL_ERROR, AIO4C_ALLOC_ERROR_TYPE, AIO4C_ALLOC_ERROR, &code);
            ReleaseLock(queue->lock);
            return false;
        }
        queue->items = items;
        queue->items[queue->maxItems] = _NewItem();
        queue->maxItems++;
    }

    memcpy(queue->items[queue->numItems], item, sizeof(QueueItem));

    queue->numItems++;

    if (item->type == AIO4C_QUEUE_ITEM_EXIT) {
        queue->exit = true;
    }

    NotifyCondition(queue->condition);

    ReleaseLock(queue->lock);

    return true;
}

aio4c_bool_t EnqueueDataItem(Queue* queue, void* data) {
    QueueItem item;

    memset(&item, 0, sizeof(QueueItem));

    item.type = AIO4C_QUEUE_ITEM_DATA;
    item.content.data = data;

    return _Enqueue(queue, &item);
}

aio4c_bool_t EnqueueExitItem(Queue* queue) {
    QueueItem item;

    memset(&item, 0, sizeof(QueueItem));

    item.type = AIO4C_QUEUE_ITEM_EXIT;

    return _Enqueue(queue, &item);
}

aio4c_bool_t EnqueueEventItem(Queue* queue, Event type, void* source) {
    QueueItem item;

    memset(&item, 0, sizeof(QueueItem));

    item.type = AIO4C_QUEUE_ITEM_EVENT;
    item.content.event.type = type;
    item.content.event.source = source;

    return _Enqueue(queue, &item);
}

aio4c_bool_t EnqueueTaskItem(Queue* queue, Event event, Connection* connection, Buffer* buffer) {
    QueueItem item;

    memset(&item, 0, sizeof(QueueItem));

    item.type = AIO4C_QUEUE_ITEM_TASK;
    item.content.task.event = event;
    item.content.task.connection = connection;
    item.content.task.buffer = buffer;

    return _Enqueue(queue, &item);
}

aio4c_bool_t RemoveAll(Queue* queue, aio4c_bool_t (*removeCallback)(QueueItem*,void*), void* discriminant) {
    int i = 0, j = 0;
    aio4c_bool_t removed = false;
    QueueItem* toRemove = NULL;

    TakeLock(queue->lock);

    NotifyCondition(queue->condition);

    for (i = 0; i < queue->numItems; ) {
        if (removeCallback(queue->items[i], discriminant)) {
            toRemove = queue->items[i];
            memset(toRemove, 0, sizeof(QueueItem));

            for (j = i; j < queue->numItems - 1; j++) {
                queue->items[j] = queue->items[j + 1];
            }

            queue->items[j] = toRemove;
            queue->numItems--;

            removed = true;
        } else {
            i++;
        }
    }

    ReleaseLock(queue->lock);

    return removed;
}

void FreeQueue(Queue** queue) {
    Queue* pQueue = NULL;
    int i = 0;

    if (queue != NULL && (pQueue = *queue) != NULL) {
        TakeLock(pQueue->lock);

        pQueue->valid = false;

        NotifyCondition(pQueue->condition);

        if (pQueue->items != NULL) {
            for (i = 0; i < pQueue->maxItems; i++) {
                if (pQueue->items[i] != NULL) {
                    aio4c_free(pQueue->items[i]);
                    pQueue->items[i] = NULL;
                }
            }
            aio4c_free(pQueue->items);
            pQueue->items = NULL;
        }

        pQueue->numItems = 0;
        pQueue->maxItems = 0;

        FreeCondition(&pQueue->condition);

        ReleaseLock(pQueue->lock);
        FreeLock(&pQueue->lock);

        aio4c_free(pQueue);
        *queue = NULL;
    }
}

#ifdef AIO4C_WIN32
static int _selectorNextPort = 50000;
#endif /* AIO4C_WIN32 */

Selector* NewSelector(void) {
    Selector* selector = NULL;
    ErrorCode code = AIO4C_ERROR_CODE_INITIALIZER;

    if ((selector = aio4c_malloc(sizeof(Selector))) == NULL) {
#ifndef AIO4C_WIN32
        code.error = errno;
#else /* AIO4C_WIN32 */
        code.source = AIO4C_ERRNO_SOURCE_SYS;
#endif /* AIO4C_WIN32 */
        code.size = sizeof(Selector);
        code.type = "Selector";
        Raise(AIO4C_LOG_LEVEL_ERROR, AIO4C_ALLOC_ERROR_TYPE, AIO4C_ALLOC_ERROR, &code);
        return NULL;
    }

    if ((selector->polls = aio4c_malloc(sizeof(aio4c_poll_t))) == NULL) {
#ifndef AIO4C_WIN32
        code.error = errno;
#else /* AIO4C_WIN32 */
        code.source = AIO4C_ERRNO_SOURCE_SYS;
#endif /* AIO4C_WIN32 */
        code.size = sizeof(aio4c_poll_t);
        code.type = "aio4c_poll_t";
        Raise(AIO4C_LOG_LEVEL_ERROR, AIO4C_ALLOC_ERROR_TYPE, AIO4C_ALLOC_ERROR, &code);
        aio4c_free(selector);
        return NULL;
    }

#ifndef AIO4C_WIN32
    if (pipe(selector->pipe) != 0) {
        code.error = errno;
#else /* AIO4C_WIN32 */
    if ((selector->pipe[AIO4C_PIPE_READ] = socket(AF_INET, SOCK_DGRAM, 0)) == INVALID_SOCKET) {
        code.source = AIO4C_ERRNO_SOURCE_WSA;
#endif /* AIO4C_WIN32 */
        code.selector = selector;
        Raise(AIO4C_LOG_LEVEL_ERROR, AIO4C_THREAD_SELECTOR_ERROR_TYPE, AIO4C_THREAD_SELECTOR_INIT_ERROR, &code);
        aio4c_free(selector->polls);
        aio4c_free(selector);
        return NULL;
    }

#ifdef AIO4C_WIN32
    if ((selector->pipe[AIO4C_PIPE_WRITE] = socket(AF_INET, SOCK_DGRAM, 0)) == INVALID_SOCKET) {
        code.source = AIO4C_ERRNO_SOURCE_WSA;
        code.selector = selector;
        Raise(AIO4C_LOG_LEVEL_ERROR, AIO4C_THREAD_SELECTOR_ERROR_TYPE, AIO4C_THREAD_SELECTOR_INIT_ERROR, &code);
        closesocket(selector->pipe[AIO4C_PIPE_READ]);
        aio4c_free(selector->polls);
        aio4c_free(selector);
        return NULL;
    }
#endif /* AIO4C_WIN32 */

#ifndef AIO4C_WIN32
    if (fcntl(selector->pipe[AIO4C_PIPE_READ], F_SETFL, O_NONBLOCK) != 0) {
        code.error = errno;
        code.selector = selector;
        Raise(AIO4C_LOG_LEVEL_ERROR, AIO4C_THREAD_SELECTOR_ERROR_TYPE, AIO4C_THREAD_SELECTOR_INIT_ERROR, &code);
        close(selector->pipe[AIO4C_PIPE_READ]);
        close(selector->pipe[AIO4C_PIPE_WRITE]);
#else /* AIO4C_WIN32 */
    unsigned long ioctl = 1;
    if (ioctlsocket(selector->pipe[AIO4C_PIPE_READ], FIONBIO, &ioctl) != 0) {
        code.source = AIO4C_ERRNO_SOURCE_WSA;
        code.selector = selector;
        Raise(AIO4C_LOG_LEVEL_ERROR, AIO4C_THREAD_SELECTOR_ERROR_TYPE, AIO4C_THREAD_SELECTOR_INIT_ERROR, &code);
        closesocket(selector->pipe[AIO4C_PIPE_READ]);
        closesocket(selector->pipe[AIO4C_PIPE_WRITE]);
#endif /* AIO4C_WIN32 */
        aio4c_free(selector->polls);
        aio4c_free(selector);
        return NULL;
    }

#ifndef AIO4C_WIN32
    if (fcntl(selector->pipe[AIO4C_PIPE_WRITE], F_SETFL, O_NONBLOCK) != 0) {
        code.error = errno;
        code.selector = selector;
        Raise(AIO4C_LOG_LEVEL_ERROR, AIO4C_THREAD_SELECTOR_ERROR_TYPE, AIO4C_THREAD_SELECTOR_INIT_ERROR, &code);
        close(selector->pipe[AIO4C_PIPE_READ]);
        close(selector->pipe[AIO4C_PIPE_WRITE]);
#else /* AIO4C_WIN32 */
    if (ioctlsocket(selector->pipe[AIO4C_PIPE_WRITE], FIONBIO, &ioctl) != 0) {
        code.source = AIO4C_ERRNO_SOURCE_WSA;
        code.selector = selector;
        Raise(AIO4C_LOG_LEVEL_ERROR, AIO4C_THREAD_SELECTOR_ERROR_TYPE, AIO4C_THREAD_SELECTOR_INIT_ERROR, &code);
        closesocket(selector->pipe[AIO4C_PIPE_READ]);
        closesocket(selector->pipe[AIO4C_PIPE_WRITE]);
#endif /* AIO4C_WIN32 */
        aio4c_free(selector->polls);
        aio4c_free(selector);
        return NULL;
    }

#ifdef AIO4C_WIN32
    selector->port = htons(_selectorNextPort++);
#endif /* AIO4C_WIN32 */

    selector->keys = NULL;
    selector->numKeys = 0;
    selector->maxKeys = 0;

    memset(selector->polls, 0, sizeof(aio4c_poll_t));

    selector->polls[0].fd = selector->pipe[AIO4C_PIPE_READ];
    selector->polls[0].events = AIO4C_OP_READ;

    selector->numPolls = 1;
    selector->maxPolls = 1;
    selector->curKey = -1;
    selector->curKeyCount = -1;

    selector->lock = NewLock();

    return selector;
}

SelectionKey* _NewSelectionKey(void) {
    SelectionKey* key = NULL;
    ErrorCode code = AIO4C_ERROR_CODE_INITIALIZER;

    if ((key = aio4c_malloc(sizeof(SelectionKey))) == NULL) {
#ifndef AIO4C_WIN32
        code.error = errno;
#else /* AIO4C_WIN32 */
        code.source = AIO4C_ERRNO_SOURCE_SYS;
#endif /* AIO4C_WIN32 */
        code.size = sizeof(SelectionKey);
        code.type = "SelectionKey";
        Raise(AIO4C_LOG_LEVEL_ERROR, AIO4C_ALLOC_ERROR_TYPE, AIO4C_ALLOC_ERROR, &code);

        return NULL;
    }

    memset(key, 0, sizeof(SelectionKey));

    return key;
}

SelectionKey* Register(Selector* selector, SelectionOperation operation, aio4c_socket_t fd, void* attachment) {
    SelectionKey** keys = NULL;
    aio4c_poll_t* polls = NULL;
    SelectionKey* key = NULL;
    ErrorCode code = AIO4C_ERROR_CODE_INITIALIZER;
    int keyPresent = -1;
    int i = 0;

    TakeLock(selector->lock);

    SelectorWakeUp(selector);

    for (i = 0; i < selector->numKeys; i++) {
        if (selector->keys[i]->fd == fd) {
            keyPresent = i;
            break;
        }
    }

    if (keyPresent == -1) {
        if (selector->numKeys == selector->maxKeys) {
            if ((keys = aio4c_realloc(selector->keys, (selector->maxKeys + 1) * sizeof(SelectionKey*))) == NULL) {
#ifndef AIO4C_WIN32
                code.error = errno;
#else /* AIO4C_WIN32 */
                code.source = AIO4C_ERRNO_SOURCE_SYS;
#endif /* AIO4C_WIN32 */
                code.size = (selector->maxKeys + 1) * sizeof(SelectionKey*);
                code.type = "SelectionKey*";
                Raise(AIO4C_LOG_LEVEL_ERROR, AIO4C_ALLOC_ERROR_TYPE, AIO4C_ALLOC_ERROR, &code);
                ReleaseLock(selector->lock);
                return NULL;
            }

            selector->keys = keys;

            selector->keys[selector->maxKeys] = _NewSelectionKey();

            selector->maxKeys++;
        }

        if (selector->numPolls == selector->maxPolls) {
            if ((polls = aio4c_realloc(selector->polls, (selector->maxPolls + 1) * sizeof(aio4c_poll_t))) == NULL) {
#ifndef AIO4C_WIN32
                code.error = errno;
#else /* AIO4C_WIN32 */
                code.source = AIO4C_ERRNO_SOURCE_SYS;
#endif /* AIO4C_WIN32 */
                code.size = (selector->maxPolls + 1) * sizeof(aio4c_poll_t);
                code.type = "aio4c_poll_t";
                Raise(AIO4C_LOG_LEVEL_ERROR, AIO4C_ALLOC_ERROR_TYPE, AIO4C_ALLOC_ERROR, &code);
                ReleaseLock(selector->lock);
                return NULL;
            }

            selector->polls = polls;

            memset(&selector->polls[selector->maxPolls], 0, sizeof(aio4c_poll_t));

            selector->maxPolls++;
        }

        memset(selector->keys[selector->numKeys], 0, sizeof(SelectionKey));

        selector->keys[selector->numKeys]->index = selector->numKeys;
        selector->keys[selector->numKeys]->operation = operation;
        selector->keys[selector->numKeys]->fd = fd;
        selector->keys[selector->numKeys]->attachment = attachment;
        keyPresent = selector->numKeys;

        memset(&selector->polls[selector->numPolls], 0, sizeof(aio4c_poll_t));
        selector->polls[selector->numPolls].fd = fd;
        selector->polls[selector->numPolls].events = operation;
        selector->keys[selector->numKeys]->poll = selector->numPolls;
        selector->numPolls++;

        selector->numKeys++;
    }

    key = selector->keys[keyPresent];
    selector->keys[keyPresent]->count++;

    ReleaseLock(selector->lock);

    return key;
}

void Unregister(Selector* selector, SelectionKey* key, aio4c_bool_t unregisterAll) {
    int i = 0, j = 0;
    aio4c_size_t pollIndex = key->poll;

    TakeLock(selector->lock);

    SelectorWakeUp(selector);

    if (key->index == -1) {
        ReleaseLock(selector->lock);
        return;
    }

    if (!unregisterAll) {
        key->count--;

        if (key->count > 0) {
            ReleaseLock(selector->lock);
            return;
        }
    }

    for (i = key->index; i < selector->numKeys - 1; i++) {
        memcpy(&selector->keys[i], &selector->keys[i + 1], sizeof(SelectionKey*));
        selector->keys[i]->index = i;
    }

    selector->keys[i] = key;
    memset(selector->keys[i], 0, sizeof(SelectionKey));
    selector->keys[i]->index = -1;

    selector->numKeys--;

    for (i = pollIndex; i < selector->numPolls - 1; i++) {
        for (j = 0; j < selector->numKeys; j++) {
            if (selector->keys[j]->poll == i + 1) {
                selector->keys[j]->poll--;
            }
        }

        memcpy(&selector->polls[i], &selector->polls[i + 1], sizeof(aio4c_poll_t));
    }

    memset(&selector->polls[i], 0, sizeof(aio4c_poll_t));

    selector->numPolls--;

    ReleaseLock(selector->lock);
}

aio4c_size_t Select(Selector* selector) {
    int nbPolls = 0;
    unsigned char dummy = 0;
    ErrorCode code = AIO4C_ERROR_CODE_INITIALIZER;

#ifndef AIO4C_WIN32
    while ((nbPolls = poll(selector->polls, selector->numPolls, -1)) < 0) {
        if (errno != EINTR) {
            code.error = errno;
            code.selector = selector;
            Raise(AIO4C_LOG_LEVEL_WARN, AIO4C_THREAD_SELECTOR_ERROR_TYPE, AIO4C_THREAD_SELECTOR_SELECT_ERROR, &code);
            return 0;
        }
#else /* AIO4C_WIN32 */
    while ((nbPolls = WSAPoll(selector->polls, selector->numPolls, -1)) == SOCKET_ERROR) {
        code.source = AIO4C_ERRNO_SOURCE_WSA;
        code.selector = selector;
        Raise(AIO4C_LOG_LEVEL_WARN, AIO4C_THREAD_SELECTOR_ERROR_TYPE, AIO4C_THREAD_SELECTOR_SELECT_ERROR, &code);
        return 0;
#endif /* AIO4C_WIN32 */
    }

    TakeLock(selector->lock);

    if (selector->polls[0].revents & AIO4C_OP_READ) {
#ifndef AIO4C_WIN32
        if (read(selector->pipe[AIO4C_PIPE_READ], &dummy, sizeof(unsigned char)) < 0) {
            code.error = errno;
#else /* AIO4C_WIN32 */
        if (recv(selector->pipe[AIO4C_PIPE_READ], (void*)&dummy, sizeof(unsigned char), 0) == SOCKET_ERROR) {
            code.source= AIO4C_ERRNO_SOURCE_WSA;
#endif /* AIO4C_WIN32 */
            code.selector = selector;
            Raise(AIO4C_LOG_LEVEL_WARN, AIO4C_THREAD_SELECTOR_ERROR_TYPE, AIO4C_THREAD_SELECTOR_SELECT_ERROR, &code);
        }
        selector->polls[0].revents = 0;
        nbPolls--;
    }

    ReleaseLock(selector->lock);

    return nbPolls;
}

void SelectorWakeUp(Selector* selector) {
    unsigned char dummy = 1;
    ErrorCode code = AIO4C_ERROR_CODE_INITIALIZER;
    aio4c_poll_t polls[1] = { { .fd = selector->pipe[AIO4C_PIPE_WRITE], .events = AIO4C_OP_WRITE, .revents = 0 } };

#ifndef AIO4C_WIN32
    if (poll(polls, 1, 0) < 0) {
        code.error = errno;
#else /* AIO4C_WIN32 */
    if (WSAPoll(polls, 1, 0) == SOCKET_ERROR) {
        code.source = AIO4C_ERRNO_SOURCE_WSA;
#endif /* AIO4C_WIN32 */
        code.selector = selector;
        Raise(AIO4C_LOG_LEVEL_WARN, AIO4C_THREAD_SELECTOR_ERROR_TYPE, AIO4C_THREAD_SELECTOR_WAKE_UP_ERROR, &code);
        return;
    }

    if (polls[0].revents & AIO4C_OP_WRITE) {
#ifndef AIO4C_WIN32
        if ((write(selector->pipe[AIO4C_PIPE_WRITE], &dummy, sizeof(unsigned char))) < 0) {
            code.error = errno;
#else /* AIO4C_WIN32 */
        struct sockaddr_in sa = { .sin_family = AF_INET, .sin_port = selector->port, .sin_addr = { .s_addr = INADDR_LOOPBACK } };
        if (sendto(selector->pipe[AIO4C_PIPE_WRITE], (void*)&dummy, sizeof(unsigned char), 0, (struct sockaddr*)&sa, sizeof(struct sockaddr_in)) == SOCKET_ERROR) {
            code.source = AIO4C_ERRNO_SOURCE_WSA;
#endif /* AIO4C_WIN32 */
            code.selector = selector;
            Raise(AIO4C_LOG_LEVEL_WARN, AIO4C_THREAD_SELECTOR_ERROR_TYPE, AIO4C_THREAD_SELECTOR_WAKE_UP_ERROR, &code);
        }
    }
}

void FreeSelector(Selector** pSelector) {
    Selector* selector = NULL;
    int i = 0;

    if (pSelector != NULL && (selector = *pSelector) != NULL) {
#ifndef AIO4C_WIN32
        close(selector->pipe[AIO4C_PIPE_WRITE]);
        close(selector->pipe[AIO4C_PIPE_READ]);
#else /* AIO4C_WIN32 */
        closesocket(selector->pipe[AIO4C_PIPE_WRITE]);
        closesocket(selector->pipe[AIO4C_PIPE_READ]);
#endif /* AIO4C_WIN32 */
        if (selector->keys != NULL) {
            for (i = 0; i < selector->maxKeys; i++) {
                if (selector->keys[i] != NULL) {
                    aio4c_free(selector->keys[i]);
                }
            }
            aio4c_free(selector->keys);
        }
        selector->numKeys = 0;
        selector->maxKeys = 0;
        selector->curKey = 0;
        if (selector->polls != NULL) {
            aio4c_free(selector->polls);
        }
        selector->numPolls = 0;
        selector->maxPolls = 0;
        FreeLock(&selector->lock);
        aio4c_free(selector);
        *pSelector = NULL;
    }
}

aio4c_bool_t SelectionKeyReady (Selector* selector, SelectionKey** key) {
    int i = 0;
    aio4c_bool_t result = true;
    aio4c_poll_t polls[1] = { { .fd = 0, .events = 0, .revents = 0 } };
    ErrorCode code = AIO4C_ERROR_CODE_INITIALIZER;

    TakeLock(selector->lock);

    if (selector->curKey == -1) {
        selector->curKey = 0;
        selector->curKeyCount = 0;
    }

    if (selector->curKey >= selector->numKeys) {
        selector->curKey = -1;
        selector->curKeyCount = -1;
        result = false;
    }

    if (result) {
        if (selector->curKeyCount >= 1 && selector->keys[selector->curKey]->count > selector->curKeyCount) {
            polls[0].fd = selector->keys[selector->curKey]->fd;
            polls[0].events = selector->keys[selector->curKey]->operation;
#ifndef AIO4C_WIN32
            if (poll(polls, 1, 0) < 0) {
                code.error = errno;
#else /* AIO4C_WIN32 */
            if (WSAPoll(polls, 1, 0) == SOCKET_ERROR) {
                code.source = AIO4C_ERRNO_SOURCE_WSA;
#endif /* AIO4C_WIN32 */
                code.selector = selector;
                Raise(AIO4C_LOG_LEVEL_ERROR, AIO4C_THREAD_SELECTOR_ERROR_TYPE, AIO4C_THREAD_SELECTOR_SELECT_ERROR, &code);
            } else if (polls[0].revents & polls[0].events) {
                selector->keys[selector->curKey]->result = polls[0].revents;
                selector->curKeyCount++;
                *key = selector->keys[selector->curKey];
                return true;
            } else {
                selector->curKeyCount = 0;
                selector->curKey++;
            }
        }

        for (i = selector->curKey; i < selector->numKeys; i++) {
            if (selector->polls[selector->keys[i]->poll].revents > 0) {
                selector->keys[i]->result = selector->polls[selector->keys[i]->poll].revents;
                *key = selector->keys[i];
                selector->curKeyCount++;
                selector->curKey = i + 1;
                selector->polls[selector->keys[i]->poll].revents = 0;
                break;
            }
        }

        if (i == selector->numKeys) {
            selector->curKey = -1;
            result = false;
        }
    }

    ReleaseLock(selector->lock);

    return result;
}

static Thread* _MainThread = NULL;

static void _ThreadCleanup(Thread* thread) {
    int i = 0;

    if (thread->exit != NULL) {
        thread->exit(thread->arg);
    }

    thread->state = AIO4C_THREAD_STATE_EXITED;

    _numThreadsRunning--;

    ReleaseLock(thread->lock);

    for (i = 0; i < _numThreads; i++) {
        if (_threads[i] == thread) {
            _threads[i] = NULL;
        }
    }
}

static Thread* _runThread(Thread* thread) {
    TakeLock(thread->lock);

    _numThreadsRunning++;

    thread->running = true;

    if (thread->init != NULL) {
        thread->init(thread->arg);
    }

#ifndef AIO4C_WIN32
    pthread_cleanup_push((void(*)(void*))_ThreadCleanup, (void*)thread);
#else /* AIO4C_WIN32 */
#error "pthread_cleanup_push not implemented for win32"
#endif /* AIO4C_WIN32 */

    while (thread->running) {
        ReleaseLock(thread->lock);

        if (!thread->run(thread->arg)) {
            TakeLock(thread->lock);
            break;
        }

        TakeLock(thread->lock);
    }

#ifndef AIO4C_WIN32
    pthread_cleanup_pop(1);
#else /* AIO4C_WIN32 */
#error "pthread_cleanup_pop not implemented for win32"
#endif /* AIO4C_WIN32 */

#ifndef AIO4C_WIN32
    pthread_exit(thread);
#else /* AIO4C_WIN32 */
#error "pthread_exit not implemented for win32"
#endif /* AIO4C_WIN32 */

    return thread;
}

Thread* ThreadMain(char* name) {
    ErrorCode code = AIO4C_ERROR_CODE_INITIALIZER;

    if (_MainThread != NULL) {
        return NULL;
    }

    if ((_MainThread = aio4c_malloc(sizeof(Thread))) == NULL) {
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

    _threadsInitialized = true;
    memset(_threads, 0, AIO4C_MAX_THREADS * sizeof(Thread*));

    _MainThread->state = AIO4C_THREAD_STATE_RUNNING;
    _MainThread->name = name;
#ifndef AIO4C_WIN32
    _MainThread->id = pthread_self();
#else /* AIO4C_WIN32 */
#error "pthread_self not implemented for win32"
#endif /* AIO4C_WIN32 */
    _MainThread->lock = NewLock();
    _MainThread->run = NULL;
    _MainThread->exit = NULL;
    _MainThread->init = NULL;
    _MainThread->arg = NULL;
    _MainThread->running = true;

    _threads[0] = _MainThread;
    _numThreads++;

#ifdef AIO4C_WIN32
    WORD v = MAKEWORD(2,2);
    WSADATA d;
    if (WSAStartup(v, &d) != 0) {
        Log(_MainThread, AIO4C_LOG_LEVEL_FATAL, "cannot start winsock: %s", strerror(WSAGetLastError()));
        return NULL;
    }
#endif /* AIO4C_WIN32 */

    return _MainThread;
}

Thread* ThreadSelf(void) {
    int i = 0;

    if (_threadsInitialized == false) {
        return NULL;
    }

    for (i = 0; i < _numThreads; i++) {
        if (_threads[i] != NULL) {
#ifndef AIO4C_WIN32
            pthread_t tid = pthread_self();
            if (memcmp(&_threads[i]->id, &tid, sizeof(pthread_t)) == 0) {
                return _threads[i];
            }
#else /* AIO4C_WIN32 */
#error "pthread_self not implemented for win32"
#endif /* AIO4C_WIN32 */
        }
    }

    return NULL;
}

Thread* NewThread(char* name, void (*init)(void*), aio4c_bool_t (*run)(void*), void (*exit)(void*), void* arg) {
    Thread* thread = NULL;
    ErrorCode code = AIO4C_ERROR_CODE_INITIALIZER;

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

    if (_numThreads >= AIO4C_MAX_THREADS) {
        Log(ThreadSelf(), AIO4C_LOG_LEVEL_FATAL, "too much threads (%d), cannot create more", _numThreads);
        aio4c_free(thread);
        return NULL;
    }

    thread->name = name;
    thread->state = AIO4C_THREAD_STATE_RUNNING;
    thread->lock = NewLock();
    thread->init = init;
    thread->run = run;
    thread->exit = exit;
    thread->arg = arg;

    TakeLock(thread->lock);

#ifndef AIO4C_WIN32
    if ((code.error = pthread_create(&thread->id, NULL, (void*(*)(void*))_runThread, (void*)thread)) != 0) {
        code.thread = thread;
        Raise(AIO4C_LOG_LEVEL_ERROR, AIO4C_THREAD_ERROR_TYPE, AIO4C_THREAD_CREATE_ERROR, &code);
        ReleaseLock(thread->lock);
        FreeLock(&thread->lock);
        aio4c_free(thread);
        return NULL;
    }
#else /* AIO4C_WIN32 */
#error "pthread_create not implemented for win32"
#endif /* AIO4C_WIN32 */

    _threads[_numThreads++] = thread;

    ReleaseLock(thread->lock);

    return thread;
}

aio4c_bool_t ThreadRunning(Thread* thread) {
    aio4c_bool_t running = false;

    if (thread != NULL) {
        running = (running || thread->state == AIO4C_THREAD_STATE_RUNNING);
        running = (running || thread->state == AIO4C_THREAD_STATE_IDLE);
        running = (running || thread->state == AIO4C_THREAD_STATE_BLOCKED);
        running = (running && thread->running);
    }

    return running;
}

Thread* ThreadCancel(Thread* thread, aio4c_bool_t force) {
    ErrorCode code = AIO4C_ERROR_CODE_INITIALIZER;

    TakeLock(thread->lock);

    if (ThreadRunning(thread)) {
        thread->running = false;

        if (force) {
#ifndef AIO4C_WIN32
            if ((code.error = pthread_cancel(thread->id)) != 0) {
                code.thread = thread;
                Raise(AIO4C_LOG_LEVEL_ERROR, AIO4C_THREAD_ERROR_TYPE, AIO4C_THREAD_CANCEL_ERROR, &code);
            }
#else /* AIO4C_WIN32 */
#error "pthread_cancel not implemented for win32"
#endif /* AIO4C_WIN32 */
        }
    }

    ReleaseLock(thread->lock);

    return thread;
}

Thread* ThreadJoin(Thread* thread) {
    ErrorCode code = AIO4C_ERROR_CODE_INITIALIZER;
    void* returnValue = NULL;

    if (thread->state == AIO4C_THREAD_STATE_JOINED) {
        return thread;
    }

    _numThreadsRunning--;

#ifndef AIO4C_WIN32
    if ((code.error = pthread_join(thread->id, &returnValue)) != 0) {
        code.thread = thread;
        Raise(AIO4C_LOG_LEVEL_ERROR, AIO4C_THREAD_ERROR_TYPE, AIO4C_THREAD_JOIN_ERROR, &code);
    }
#else /* AIO4C_WIN32 */
#error "pthread_join not implemented for win32"
#endif /* AIO4C_WIN32 */

    _numThreadsRunning++;

    TakeLock(thread->lock);

    thread->state = AIO4C_THREAD_STATE_JOINED;

    ReleaseLock(thread->lock);

    return thread;
}

void FreeThread(Thread** thread) {
    Thread* pThread = NULL;

    if (thread != NULL && (pThread = *thread) != NULL) {
        if (pThread != _MainThread) {
            if (pThread->state != AIO4C_THREAD_STATE_JOINED) {
                if (pThread->state != AIO4C_THREAD_STATE_EXITED) {
                    ThreadCancel(pThread, true);
                }

                ThreadJoin(pThread);
            }
        }

#ifdef AIO4C_WIN32
        if (pThread == _MainThread) {
            WSACleanup();
        }
#endif /* AIO4C_WIN32 */

        FreeLock(&pThread->lock);
        aio4c_free(pThread);

        *thread = NULL;
    }
}

