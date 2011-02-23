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

#include <aio4c/log.h>
#include <aio4c/types.h>

#include <errno.h>
#include <fcntl.h>
#include <poll.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

static Thread*      _threads[AIO4C_MAX_THREADS];
static aio4c_bool_t _threadsInitialized = false;
static int          _numThreads = 0;

Lock* NewLock(void) {
    Lock* pLock = NULL;

    if ((pLock = malloc(sizeof(Lock))) == NULL) {
        debugLock("cannot allocate lock: %s\n", strerror(errno));
        return NULL;
    }

    pLock->state = FREE;
    pLock->owner = NULL;

    if ((errno = aio4c_mutex_init(&pLock->mutex, NULL)) != 0) {
        debugLock("cannot initialize lock: %s\n", strerror(errno));
        free(pLock);
        return NULL;
    }

    pLock->state = FREE;

    return pLock;
}

Lock* TakeLock(Lock* lock) {
    if ((errno = aio4c_mutex_lock(&lock->mutex)) != 0) {
        return lock;
    }

    return lock;
}

Lock* ReleaseLock(Lock* lock) {
    if ((errno = aio4c_mutex_unlock(&lock->mutex)) != 0) {
        return NULL;
    }
    return lock;
}

void FreeLock(Lock** lock) {
    Lock * pLock = NULL;

    if (lock != NULL && (pLock = *lock) != NULL) {
        pLock->state = DESTROYED;

        aio4c_mutex_destroy(&pLock->mutex);

        free(pLock);
        *lock = NULL;
    }
}

Condition* NewCondition(void) {
    Condition* condition = NULL;

    if ((condition = malloc(sizeof(Condition))) == NULL) {
        return NULL;
    }

    condition->owner = NULL;

    if (aio4c_cond_init(&condition->condition, NULL) != 0) {
        free(condition);
        return NULL;
    }

    return condition;
}

aio4c_bool_t WaitCondition(Condition* condition, Lock* lock) {
    if (aio4c_cond_wait(&condition->condition, &lock->mutex) != 0) {
        return false;
    }

    return true;
}

void NotifyCondition(Condition* condition) {
    aio4c_cond_signal(&condition->condition);
}

void FreeCondition(Condition** condition) {
    Condition* pCondition = NULL;

    if (condition != NULL && (pCondition = *condition) != NULL) {
        aio4c_cond_destroy(&pCondition->condition);

        free(pCondition);
        *condition = NULL;
    }
}

Queue* NewQueue(void) {
    Queue* queue = NULL;

    if ((queue = malloc(sizeof(Queue))) == NULL) {
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

QueueItem* NewDataItem(void* data) {
    QueueItem* item = NULL;

    if ((item = malloc(sizeof(QueueItem))) == NULL) {
        return NULL;
    }

    item->type = DATA;
    item->content.data = data;

    return item;
}

QueueItem* NewEventItem(Event event, void* source) {
    QueueItem* item = NULL;

    if ((item = malloc(sizeof(QueueItem))) == NULL) {
        return NULL;
    }

    item->type = EVENT;
    item->content.event.type = event;
    item->content.event.source = source;

    return item;
}

QueueItem* NewExitItem(void) {
    QueueItem* item = NULL;

    if ((item = malloc(sizeof(QueueItem))) == NULL) {
        return NULL;
    }

    item->type = EXIT;

    return item;
}


void FreeItem(QueueItem** pItem) {
    QueueItem* item = NULL;
    if (pItem != NULL && (item = *pItem) != NULL) {
        free(item);
        *pItem = NULL;
    }
}

aio4c_bool_t Dequeue(Queue* queue, QueueItem** item, aio4c_bool_t wait) {
    int i = 0;

    TakeLock(queue->lock);

    if (queue->emptied) {
        queue->emptied = false;
        ReleaseLock(queue->lock);
        return false;
    }

    while (!queue->exit && wait && queue->numItems == 0 && queue->valid) {
        if (!WaitCondition(queue->condition, queue->lock)) {
            break;
        }
    }

    if (queue->numItems > 0) {
        *item = queue->items[0];

        for (i = 0; i < queue->numItems - 1; i++) {
            queue->items[i] = queue->items[i + 1];
        }

        queue->items[i] = NULL;
        queue->numItems--;
        queue->emptied = (queue->numItems == 0);
    } else {
        *item = NULL;
    }

    ReleaseLock(queue->lock);

    return (*item != NULL);
}

aio4c_bool_t Enqueue(Queue* queue, QueueItem* item) {
    QueueItem** items = NULL;
    TakeLock(queue->lock);

    if (queue->exit) {
        ReleaseLock(queue->lock);
        return false;
    }

    if (queue->numItems == queue->maxItems) {
        if ((items = realloc(queue->items, (queue->maxItems + 1) * sizeof(QueueItem*))) == NULL) {
            ReleaseLock(queue->lock);
            return false;
        }
        queue->items = items;
        queue->maxItems++;
    }

    queue->items[queue->numItems] = item;
    queue->numItems++;

    if (item->type == EXIT) {
        queue->exit = true;
    }

    NotifyCondition(queue->condition);

    ReleaseLock(queue->lock);

    return true;
}

aio4c_bool_t RemoveAll(Queue* queue, aio4c_bool_t (*removeCallback)(QueueItem*,void*), void* discriminant) {
    int i = 0;
    aio4c_bool_t removed = false;

    TakeLock(queue->lock);

    for (i = 0; i < queue->numItems;) {
        if (removeCallback(queue->items[i], discriminant)) {
            FreeItem(&queue->items[i]);
            if (i < queue->numItems - 1) {
                queue->items[i] = queue->items[i + 1];
            } else {
                queue->items[i] = NULL;
            }
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

    if (queue != NULL && (pQueue = *queue) != NULL) {
        TakeLock(pQueue->lock);

        pQueue->valid = false;

        NotifyCondition(pQueue->condition);

        free(pQueue->items);
        pQueue->numItems = 0;
        pQueue->maxItems = 0;

        FreeCondition(&pQueue->condition);
        FreeLock(&pQueue->lock);

        free(pQueue);
        *queue = NULL;
    }
}

Selector* NewSelector(void) {
    Selector* selector = NULL;

    if ((selector = malloc(sizeof(Selector))) == NULL) {
        return NULL;
    }

    if (pipe(selector->pipe) != 0) {
        free(selector);
        return NULL;
    }

    if (fcntl(selector->pipe[AIO4C_PIPE_READ], F_SETFL, O_NONBLOCK) != 0) {
        close(selector->pipe[AIO4C_PIPE_READ]);
        close(selector->pipe[AIO4C_PIPE_WRITE]);
        free(selector);
        return NULL;
    }

    if (fcntl(selector->pipe[AIO4C_PIPE_WRITE], F_SETFL, O_NONBLOCK) != 0) {
        close(selector->pipe[AIO4C_PIPE_READ]);
        close(selector->pipe[AIO4C_PIPE_WRITE]);
        free(selector);
        return NULL;
    }

    selector->keys = NULL;
    selector->numKeys = 0;
    selector->maxKeys = 0;

    if ((selector->polls = malloc(sizeof(aio4c_poll_t))) == NULL) {
        close(selector->pipe[AIO4C_PIPE_READ]);
        close(selector->pipe[AIO4C_PIPE_WRITE]);
        free(selector);
        return NULL;
    }

    memset(selector->polls, 0, sizeof(aio4c_poll_t));

    selector->polls[0].fd = selector->pipe[AIO4C_PIPE_READ];
    selector->polls[0].events = AIO4C_OP_READ;

    selector->numPolls = 1;
    selector->maxPolls = 1;
    selector->curKey = -1;

    selector->lock = NewLock();

    return selector;
}

SelectionKey* _NewSelectionKey(void) {
    SelectionKey* key = NULL;

    if ((key = malloc(sizeof(SelectionKey))) == NULL) {
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
            if ((keys = realloc(selector->keys, (selector->maxKeys + 1) * sizeof(SelectionKey*))) == NULL) {
                ReleaseLock(selector->lock);
                return NULL;
            }

            selector->keys = keys;

            selector->keys[selector->maxKeys] = _NewSelectionKey();

            selector->maxKeys++;
        }

        if (selector->numPolls == selector->maxPolls) {
            if ((polls = realloc(selector->polls, (selector->maxPolls + 1) * sizeof(aio4c_poll_t))) == NULL) {
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

    while ((nbPolls = poll(selector->polls, selector->numPolls, -1)) < 0) {
        if (errno != EINTR) {
            Log(ThreadSelf(), WARN, "poll failed with %s", strerror(errno));
            return 0;
        }
    }

    TakeLock(selector->lock);

    if (selector->polls[0].revents == POLLIN) {
        read(selector->pipe[AIO4C_PIPE_READ], &dummy, sizeof(unsigned char));
        selector->polls[0].revents = 0;
        nbPolls--;
    }

    ReleaseLock(selector->lock);

    return nbPolls;
}

void SelectorWakeUp(Selector* selector) {
    unsigned char dummy = 1;
    aio4c_poll_t polls[1] = { { .fd = selector->pipe[AIO4C_PIPE_WRITE], .events = AIO4C_OP_WRITE, .revents = 0 } };

    if (poll(polls, 1, 0) > 0 && polls[0].revents == AIO4C_OP_WRITE) {
        write(selector->pipe[AIO4C_PIPE_WRITE], &dummy, sizeof(unsigned char));
    }
}

void FreeSelector(Selector** pSelector) {
    Selector* selector = NULL;
    int i = 0;

    if (pSelector != NULL && (selector = *pSelector) != NULL) {
        close(selector->pipe[AIO4C_PIPE_WRITE]);
        close(selector->pipe[AIO4C_PIPE_READ]);
        for (i = 0; i < selector->maxKeys; i++) {
            free(selector->keys[i]);
        }
        free(selector->keys);
        selector->numKeys = 0;
        selector->maxKeys = 0;
        selector->curKey = 0;
        free(selector->polls);
        selector->numPolls = 0;
        selector->maxPolls = 0;
        FreeLock(&selector->lock);
        free(selector);
        *pSelector = NULL;
    }
}

aio4c_bool_t SelectionKeyReady (Selector* selector, SelectionKey** key) {
    int i = 0;
    aio4c_bool_t result = true;

    TakeLock(selector->lock);

    if (selector->curKey == -1) {
        selector->curKey = 0;
    }

    if (selector->curKey >= selector->numKeys) {
        selector->curKey = -1;
        result = false;
    }

    if (result) {
        for (i = selector->curKey; i < selector->numKeys; i++) {
            if (selector->polls[selector->keys[i]->poll].revents > 0) {
                selector->keys[i]->result = selector->polls[selector->keys[i]->poll].revents;
                *key = selector->keys[i];
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

static void _ThreadCleanup(Thread* thread) {
    int i = 0;
    thread->state = EXITED;

    if (thread->exit != NULL) {
        thread->exit(thread->arg);
    }

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

    if (thread->init != NULL) {
        thread->init(thread->arg);
    }

    aio4c_thread_cleanup_push((void(*)(void*))_ThreadCleanup, (void*)thread);

    while (thread->state != STOPPED) {
        ReleaseLock(thread->lock);

        if (!thread->run(thread->arg)) {
            TakeLock(thread->lock);
            break;
        }

        TakeLock(thread->lock);
    }


    aio4c_thread_cleanup_pop(1);

    aio4c_thread_exit(thread);
}

extern int main (int argc, char** argv);

static Thread* _MainThread = NULL;

Thread* ThreadMain(char* name) {
    if (_MainThread != NULL) {
        return _MainThread;
    }

    _threadsInitialized = true;
    memset(_threads, 0, AIO4C_MAX_THREADS * sizeof(Thread*));

    if ((_MainThread = malloc(sizeof(Thread))) == NULL) {
        return NULL;
    }

    _MainThread->state = RUNNING;
    _MainThread->name = name;
    _MainThread->id = aio4c_thread_self();
    _MainThread->lock = NewLock();
    _MainThread->run = (aio4c_bool_t(*)(void*))main;
    _MainThread->exit = NULL;
    _MainThread->init = NULL;
    _MainThread->arg = NULL;
    _MainThread->stateValid = true;

    _threads[0] = _MainThread;
    _numThreads++;

    return _MainThread;
}

Thread* ThreadSelf(void) {
    int i = 0;

    if (_threadsInitialized == false) {
        return NULL;
    }

    for (i = 0; i < _numThreads; i++) {
        if (_threads[i] != NULL) {
            if (_threads[i]->id == aio4c_thread_self()) {
                return _threads[i];
            }
        }
    }

    return NULL;
}

Thread* NewThread(char* name, void (*init)(void*), aio4c_bool_t (*run)(void*), void (*exit)(void*), void* arg) {
    Thread* thread = NULL;

    if ((thread = malloc(sizeof(Thread))) == NULL) {
        return NULL;
    }

    if (_numThreads >= AIO4C_MAX_THREADS) {
        return NULL;
    }

    thread->name = name;
    thread->state = RUNNING;
    thread->lock = NewLock();
    thread->init = init;
    thread->run = run;
    thread->exit = exit;
    thread->arg = arg;
    thread->stateValid = true;

    TakeLock(thread->lock);

    if (aio4c_thread_create(&thread->id, NULL, (void*(*)(void*))_runThread, (void*)thread) != 0) {
        ReleaseLock(thread->lock);
        FreeLock(&thread->lock);
        free(thread);
        return NULL;
    }

    _threads[_numThreads++] = thread;

    ReleaseLock(thread->lock);

    return thread;
}

aio4c_bool_t ThreadRunning(Thread* thread) {
    aio4c_bool_t running = false;

    if (thread != NULL) {
        running = (running || thread->state == RUNNING);
        running = (running || thread->state == IDLE);
        running = (running || thread->state == BLOCKED);
        running = (running && thread->stateValid);
    }

    return running;
}

Thread* ThreadCancel(Thread* thread, aio4c_bool_t force) {
    TakeLock(thread->lock);

    if (ThreadRunning(thread)) {
        if (force) {
            aio4c_thread_cancel(thread->id);
            thread->state = STOPPED;
        } else {
            thread->state = STOPPED;
        }
    }

    ReleaseLock(thread->lock);

    return thread;
}

Thread* ThreadJoin(Thread* thread) {
    void* returnValue = NULL;

    if (thread->state == JOINED) {
        return thread;
    }

    aio4c_thread_join(thread->id, &returnValue);

    aio4c_thread_detach(thread->id);

    TakeLock(thread->lock);

    thread->state = JOINED;

    ReleaseLock(thread->lock);

    return thread;
}

void FreeThread(Thread** thread) {
    Thread* pThread = NULL;
    Thread* parent = ThreadSelf();

    if (thread != NULL && (pThread = *thread) != NULL) {
        TakeLock(pThread->lock);

        if (pThread != _MainThread) {
            if (pThread->state != JOINED && parent != NULL) {
                if (pThread->state != EXITED) {
                    ThreadCancel(pThread, true);
                }

                ReleaseLock(pThread->lock);

                ThreadJoin(pThread);
            }
        }

        FreeLock(&pThread->lock);
        free(pThread);

        *thread = NULL;
    }
}

