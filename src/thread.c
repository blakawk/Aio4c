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

static Thread*      _threads[AIO4C_MAX_THREADS];
static aio4c_bool_t _threadsInitialized = false;
static int          _numThreads = 0;
static int          _numThreadsRunning = 0;

int GetNumThreads(void) {
    return _numThreadsRunning;
}

Lock* NewLock(void) {
    Lock* pLock = NULL;

    if ((pLock = aio4c_malloc(sizeof(Lock))) == NULL) {
        return NULL;
    }

    pLock->state = AIO4C_LOCK_STATE_FREE;
    pLock->owner = NULL;

#ifndef AIO4C_WIN32
    if ((errno = pthread_mutex_init(&pLock->mutex, NULL)) != 0) {
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
    ProbeTimeStart(AIO4C_TIME_PROBE_BLOCK);

#ifndef AIO4C_WIN32
    if ((errno = pthread_mutex_lock(&lock->mutex)) != 0) {
        return NULL;
    }
#else /* AIO4C_WIN32 */
#error "pthread_mutex_lock not implemented for win32"
#endif /* AIO4C_WIN32 */

    ProbeTimeEnd(AIO4C_TIME_PROBE_BLOCK);

    return lock;
}

Lock* ReleaseLock(Lock* lock) {
#ifndef AIO4C_WIN32
    if ((errno = pthread_mutex_unlock(&lock->mutex)) != 0) {
        return NULL;
    }
#else /* AIO4C_WIN32 */
#error "pthread_mutex_unlock not implemented for win32"
#endif /* AIO4C_WIN32 */
    return lock;
}

void FreeLock(Lock** lock) {
    Lock * pLock = NULL;

    if (lock != NULL && (pLock = *lock) != NULL) {
        pLock->state = AIO4C_LOCK_STATE_DESTROYED;

#ifndef AIO4C_WIN32
        pthread_mutex_destroy(&pLock->mutex);
#else /* AIO4C_WIN32 */
#error "pthread_mutex_destroy not implemented for win32"
#endif /* AIO4C_WIN32 */

        aio4c_free(pLock);
        *lock = NULL;
    }
}

Condition* NewCondition(void) {
    Condition* condition = NULL;

    if ((condition = aio4c_malloc(sizeof(Condition))) == NULL) {
        return NULL;
    }

    condition->owner = NULL;

#ifndef AIO4C_WIN32
    if (pthread_cond_init(&condition->condition, NULL) != 0) {
        aio4c_free(condition);
        return NULL;
    }
#else /* AIO4C_WIN32 */
#error "pthread_cond_init not implemented for win32"
#endif /* AIO4C_WIN32 */

    return condition;
}

aio4c_bool_t WaitCondition(Condition* condition, Lock* lock) {
#ifndef AIO4C_WIN32
    if (pthread_cond_wait(&condition->condition, &lock->mutex) != 0) {
        return false;
    }
#else /* AIO4C_WIN32 */
#error "pthread_cond_wait not implemented for win32"
#endif /* AIO4C_WIN32 */

    return true;
}

void NotifyCondition(Condition* condition) {
#ifndef AIO4C_WIN32
    pthread_cond_signal(&condition->condition);
#else /* AIO4C_WIN32 */
#error "pthread_cond_signal not implemented for win32"
#endif /* AIO4C_WIN32 */
}

void FreeCondition(Condition** condition) {
    Condition* pCondition = NULL;

    if (condition != NULL && (pCondition = *condition) != NULL) {
#ifndef AIO4C_WIN32
        pthread_cond_destroy(&pCondition->condition);
#else /* AIO4C_WIN32 */
#error "pthread_cond_destroy not implemented for win32"
#endif /* AIO4C_WIN32 */

        aio4c_free(pCondition);
        *condition = NULL;
    }
}

Queue* NewQueue(void) {
    Queue* queue = NULL;

    if ((queue = aio4c_malloc(sizeof(Queue))) == NULL) {
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

    if ((item = aio4c_malloc(sizeof(QueueItem))) == NULL) {
        return NULL;
    }

    memset(item, 0, sizeof(QueueItem));

    return item;
}

aio4c_bool_t _Enqueue(Queue* queue, QueueItem* item) {
    QueueItem** items = NULL;
    TakeLock(queue->lock);

    if (queue->exit) {
        ReleaseLock(queue->lock);
        return false;
    }

    if (queue->numItems == queue->maxItems) {
        if ((items = aio4c_realloc(queue->items, (queue->maxItems + 1) * sizeof(QueueItem*))) == NULL) {
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

    if ((selector = aio4c_malloc(sizeof(Selector))) == NULL) {
        return NULL;
    }

    if ((selector->polls = aio4c_malloc(sizeof(aio4c_poll_t))) == NULL) {
        aio4c_free(selector);
        return NULL;
    }

#ifndef AIO4C_WIN32
    if (pipe(selector->pipe) != 0) {
#else /* AIO4C_WIN32 */
    if ((selector->pipe[AIO4C_PIPE_READ] = socket(AF_INET, SOCK_DGRAM, 0)) == INVALID_SOCKET) {
#endif /* AIO4C_WIN32 */
        aio4c_free(selector->polls);
        aio4c_free(selector);
        return NULL;
    }

#ifdef AIO4C_WIN32
    if ((selector->pipe[AIO4C_PIPE_WRITE] = socket(AF_INET, SOCK_DGRAM, 0)) == INVALID_SOCKET) {
        closesocket(selector->pipe[AIO4C_PIPE_READ]);
        aio4c_free(selector->polls);
        aio4c_free(selector);
        return NULL;
    }
#endif /* AIO4C_WIN32 */

#ifndef AIO4C_WIN32
    if (fcntl(selector->pipe[AIO4C_PIPE_READ], F_SETFL, O_NONBLOCK) != 0) {
        close(selector->pipe[AIO4C_PIPE_READ]);
        close(selector->pipe[AIO4C_PIPE_WRITE]);
#else /* AIO4C_WIN32 */
    unsigned long ioctl = 1;
    if (ioctlsocket(selector->pipe[AIO4C_PIPE_READ], FIONBIO, &ioctl) != 0) {
        closesocket(selector->pipe[AIO4C_PIPE_READ]);
        closesocket(selector->pipe[AIO4C_PIPE_WRITE]);
#endif /* AIO4C_WIN32 */
        aio4c_free(selector->polls);
        aio4c_free(selector);
        return NULL;
    }

#ifndef AIO4C_WIN32
    if (fcntl(selector->pipe[AIO4C_PIPE_WRITE], F_SETFL, O_NONBLOCK) != 0) {
        close(selector->pipe[AIO4C_PIPE_READ]);
        close(selector->pipe[AIO4C_PIPE_WRITE]);
#else /* AIO4C_WIN32 */
    if (ioctlsocket(selector->pipe[AIO4C_PIPE_WRITE], FIONBIO, &ioctl) != 0) {
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

    if ((key = aio4c_malloc(sizeof(SelectionKey))) == NULL) {
        return NULL;
    }

    memset(key, 0, sizeof(SelectionKey));

    return key;
}

SelectionKey* Register(Selector* selector, SelectionOperation operation, aio4c_socket_t fd, void* attachment) {
    SelectionKey** keys = NULL;
    aio4c_poll_t* polls = NULL;
    SelectionKey* key = NULL;
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
                ReleaseLock(selector->lock);
                return NULL;
            }

            selector->keys = keys;

            selector->keys[selector->maxKeys] = _NewSelectionKey();

            selector->maxKeys++;
        }

        if (selector->numPolls == selector->maxPolls) {
            if ((polls = aio4c_realloc(selector->polls, (selector->maxPolls + 1) * sizeof(aio4c_poll_t))) == NULL) {
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

#ifndef AIO4C_WIN32
    while ((nbPolls = poll(selector->polls, selector->numPolls, -1)) < 0) {
        if (errno != EINTR) {
            Log(ThreadSelf(), AIO4C_LOG_LEVEL_WARN, "poll failed with %s", strerror(errno));
            return 0;
        }
#else /* AIO4C_WIN32 */
    while ((nbPolls = WSAPoll(selector->polls, selector->numPolls, -1)) == SOCKET_ERROR) {
        int error = WSAGetLastError();
        Log(ThreadSelf(), AIO4C_LOG_LEVEL_WARN, "poll failed with %s", strerror(error));
        return 0;
#endif /* AIO4C_WIN32 */
    }

    TakeLock(selector->lock);

    if (selector->polls[0].revents & AIO4C_OP_READ) {
#ifndef AIO4C_WIN32
        read(selector->pipe[AIO4C_PIPE_READ], &dummy, sizeof(unsigned char));
#else /* AIO4C_WIN32 */
        recv(selector->pipe[AIO4C_PIPE_READ], (void*)&dummy, sizeof(unsigned char), 0);
#endif /* AIO4C_WIN32 */
        selector->polls[0].revents = 0;
        nbPolls--;
    }

    ReleaseLock(selector->lock);

    return nbPolls;
}

void SelectorWakeUp(Selector* selector) {
    unsigned char dummy = 1;
    aio4c_poll_t polls[1] = { { .fd = selector->pipe[AIO4C_PIPE_WRITE], .events = AIO4C_OP_WRITE, .revents = 0 } };

#ifndef AIO4C_WIN32
    if (poll(polls, 1, 0) > 0 && polls[0].revents & AIO4C_OP_WRITE) {
        write(selector->pipe[AIO4C_PIPE_WRITE], &dummy, sizeof(unsigned char));
#else /* AIO4C_WIN32 */
    if (WSAPoll(polls, 1, 0) > 0 && polls[0].revents & AIO4C_OP_WRITE) {
        struct sockaddr_in sa = { .sin_family = AF_INET, .sin_port = selector->port, .sin_addr = { .s_addr = INADDR_LOOPBACK } };
        sendto(selector->pipe[AIO4C_PIPE_WRITE], (void*)&dummy, sizeof(unsigned char), 0, (struct sockaddr*)&sa, sizeof(struct sockaddr_in));
#endif /* AIO4C_WIN32 */
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
            if (poll(polls, 1, 0) == 1 && polls[0].revents & polls[0].events) {
#else /* AIO4C_WIN32 */
            if (WSAPoll(polls, 1, 0) == 1 && polls[0].revents & polls[0].events) {
#endif /* AIO4C_WIN32 */
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
    thread->state = AIO4C_THREAD_STATE_EXITED;

    if (thread->exit != NULL) {
        thread->exit(thread->arg);
    }

    _numThreadsRunning--;

    ReleaseLock(thread->lock);

    for (i = 0; i < _numThreads; i++) {
        if (_threads[i] == thread) {
            _threads[i] = NULL;
        }
    }

    thread->stateValid = false;
}

static Thread* _runThread(Thread* thread) {
    TakeLock(thread->lock);

    _numThreadsRunning++;

    if (thread->init != NULL) {
        thread->init(thread->arg);
    }

#ifndef AIO4C_WIN32
    pthread_cleanup_push((void(*)(void*))_ThreadCleanup, (void*)thread);
#else /* AIO4C_WIN32 */
#error "pthread_cleanup_push not implemented for win32"
#endif /* AIO4C_WIN32 */

    while (thread->state != AIO4C_THREAD_STATE_STOPPED) {
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
    if (_MainThread != NULL) {
        return NULL;
    }

    if ((_MainThread = aio4c_malloc(sizeof(Thread))) == NULL) {
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
    _MainThread->stateValid = true;

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

    if ((thread = aio4c_malloc(sizeof(Thread))) == NULL) {
        return NULL;
    }

    if (_numThreads >= AIO4C_MAX_THREADS) {
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
    thread->stateValid = true;

    TakeLock(thread->lock);

#ifndef AIO4C_WIN32
    if (pthread_create(&thread->id, NULL, (void*(*)(void*))_runThread, (void*)thread) != 0) {
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
        running = (running && thread->stateValid);
    }

    return running;
}

Thread* ThreadCancel(Thread* thread, aio4c_bool_t force) {
    TakeLock(thread->lock);

    if (ThreadRunning(thread)) {
        if (force) {
#ifndef AIO4C_WIN32
            pthread_cancel(thread->id);
#else /* AIO4C_WIN32 */
#error "pthread_cancel not implemented for win32"
#endif /* AIO4C_WIN32 */
            thread->state = AIO4C_THREAD_STATE_STOPPED;
        } else {
            thread->state = AIO4C_THREAD_STATE_STOPPED;
        }
    }

    ReleaseLock(thread->lock);

    return thread;
}

Thread* ThreadJoin(Thread* thread) {
    void* returnValue = NULL;

    if (thread->state == AIO4C_THREAD_STATE_JOINED) {
        return thread;
    }

    _numThreadsRunning--;

#ifndef AIO4C_WIN32
    pthread_join(thread->id, &returnValue);
#else /* AIO4C_WIN32 */
#error "pthread_join not implemented for win32"
#endif /* AIO4C_WIN32 */

    _numThreadsRunning++;

#ifndef AIO4C_WIN32
    pthread_detach(thread->id);
#else /* AIO4C_WIN32 */
#error "pthread_detach not implemented for win32"
#endif /* AIO4C_WIN32 */

    TakeLock(thread->lock);

    thread->state = AIO4C_THREAD_STATE_JOINED;

    ReleaseLock(thread->lock);

    return thread;
}

void FreeThread(Thread** thread) {
    Thread* pThread = NULL;

    if (thread != NULL && (pThread = *thread) != NULL) {
        TakeLock(pThread->lock);

        if (pThread != _MainThread) {
            if (pThread->state != AIO4C_THREAD_STATE_JOINED) {
                if (pThread->state != AIO4C_THREAD_STATE_EXITED) {
                    ThreadCancel(pThread, true);
                }

                ReleaseLock(pThread->lock);

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

