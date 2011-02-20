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
static aio4c_size_t _numThreads = 0;

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
    Thread* owner = ThreadSelf();

    if (lock == NULL || lock->state == DESTROYED) {
        debugLock("lock %p has been destroyed\n", (void*)lock);
        return NULL;
    }

    if (owner != NULL && owner == lock->owner) {
        debugLock("lock %p is already owned by %p [%s]\n", (void*)lock, (void*)owner, owner->name);
        return NULL;
    }

    if (lock->owner != owner && lock->owner != NULL && !ThreadRunning(lock->owner)) {
        debugLock("lock %p owner %p [%s] is not running anymore\n", (void*)lock, (void*)lock->owner, lock->owner->name);
        return NULL;
    }

    if (owner != NULL) {
        owner->state = BLOCKED;
    }

    debugLock("%p [%s] locking %p\n", (void*)owner, (owner != NULL)?owner->name:NULL, (void*)lock);
    if ((errno = aio4c_mutex_lock(&lock->mutex)) != 0) {
        debugLock("error locking %p: %s", (void*)lock, strerror(errno));
        lock->owner = NULL;
        lock->state = FREE;
        if (owner->state == BLOCKED) {
            owner->state = RUNNING;
        }
        return NULL;
    }

    lock->owner = owner;
    lock->state = LOCKED;

    if (owner != NULL) {
        if (owner->state == BLOCKED) {
            owner->state = RUNNING;
        }
    }

    return lock;
}

Lock* ReleaseLock(Lock* lock) {
    Thread* releaser = ThreadSelf();
    if (lock == NULL || lock->state == DESTROYED) {
        debugLock("lock %p has been destroyed\n", (void*)lock);
        return NULL;
    }

    if (lock->owner == NULL) {
        debugLock("lock %p is already released\n", (void*)lock);
        return NULL;
    }

    debugLock("%p [%s] releasing %p owned by %p [%s]\n", (void*)releaser, releaser->name, (void*)lock, (void*)lock->owner, lock->owner->name);

    lock->owner = NULL;
    lock->state = FREE;

    if ((errno = aio4c_mutex_unlock(&lock->mutex)) != 0) {
        debugLock("error unlocking %p: %s\n", (void*)lock, strerror(errno));
        return NULL;
    }

    return lock;
}

void FreeLock(Lock** lock) {
    Lock * pLock = NULL;

    if (lock != NULL && (pLock = *lock) != NULL) {
        while (pLock->state == LOCKED) {
            ReleaseLock(pLock);
        }

        debugLock("destroying lock %p\n", (void*)lock);
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
    Thread* owner = ThreadSelf();

    if (lock == NULL || condition == NULL) {
        return false;
    }

    if ((condition->owner != NULL && condition->owner == owner)
    || ((lock->owner != NULL && lock->owner != owner)
      || lock->state != LOCKED)) {
        return false;
    }

    condition->owner = owner;

    if (owner != NULL) {
        TakeLock(owner->lock);
        if (!ThreadRunning(owner)) {
            ReleaseLock(owner->lock);
            return false;
        }
        owner->state = IDLE;
        ReleaseLock(owner->lock);
    }

    lock->state = FREE;
    lock->owner = NULL;

    if (aio4c_cond_wait(&condition->condition, &lock->mutex) != 0) {
        lock->owner = owner;
        lock->state = LOCKED;
        condition->owner = NULL;
        if (owner != NULL) {
            TakeLock(owner->lock);
            if (owner->state == IDLE) {
                owner->state = RUNNING;
            }
            ReleaseLock(owner->lock);
        }
        return false;
    }

    lock->owner = owner;
    lock->state = LOCKED;

    condition->owner = NULL;

    if (owner != NULL) {
        TakeLock(owner->lock);
        if (owner->state == IDLE) {
            owner->state = RUNNING;
        }
        ReleaseLock(owner->lock);
    }

    return true;
}

void NotifyCondition(Condition* condition, Lock* lock) {
    Thread* notifier = ThreadSelf();

    if (condition == NULL || lock == NULL) {
        return;
    }

    if (notifier == NULL || condition->owner == notifier || condition->owner == NULL || ((lock->owner != NULL && lock->owner != notifier) || lock->state != LOCKED)) {
        return;
    }

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

aio4c_bool_t Dequeue(Queue* queue, QueueItem** item) {
    int i = 0;

    TakeLock(queue->lock);

    if (queue->emptied) {
        queue->emptied = false;
        ReleaseLock(queue->lock);
        return false;
    }

    while (queue->numItems == 0 && queue->valid) {
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

    if (queue->numItems == queue->maxItems) {
        if ((items = realloc(queue->items, (queue->maxItems + 1) * sizeof(QueueItem*))) == NULL) {
            return false;
        }
        queue->items = items;
        queue->maxItems++;
    }

    queue->items[queue->numItems] = item;
    queue->numItems++;

    NotifyCondition(queue->condition, queue->lock);

    ReleaseLock(queue->lock);

    return true;
}

void FreeQueue(Queue** queue) {
    Queue* pQueue = NULL;

    if (queue != NULL && (pQueue = *queue) != NULL) {
        TakeLock(pQueue->lock);

        pQueue->valid = false;

        NotifyCondition(pQueue->condition, pQueue->lock);

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

SelectionKey* Register(Selector* selector, SelectionOperation operation, aio4c_socket_t fd, void* attachment) {
    SelectionKey* keys = NULL;
    aio4c_poll_t* polls = NULL;
    SelectionKey* key = NULL;

    TakeLock(selector->lock);

    SelectorWakeUp(selector);

    if (selector->numKeys == selector->maxKeys) {
        if ((keys = realloc(selector->keys, (selector->maxKeys + 1) * sizeof(SelectionKey))) == NULL) {
            ReleaseLock(selector->lock);
            return NULL;
        }

        selector->keys = keys;

        memset(&selector->keys[selector->maxKeys], 0, sizeof(SelectionKey));

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

    memset(&selector->keys[selector->numKeys], 0, sizeof(SelectionKey));
    selector->keys[selector->numKeys].index = selector->numKeys;
    selector->keys[selector->numKeys].operation = operation;
    selector->keys[selector->numKeys].fd = fd;
    selector->keys[selector->numKeys].attachment = attachment;
    selector->keys[selector->numKeys].poll = selector->numPolls;
    key = &selector->keys[selector->numKeys];
    selector->numKeys++;

    memset(&selector->polls[selector->numPolls], 0, sizeof(aio4c_poll_t));
    selector->polls[selector->numPolls].fd = fd;
    selector->polls[selector->numPolls].events = operation;
    selector->numPolls++;

    ReleaseLock(selector->lock);

    return key;
}

void Unregister(Selector* selector, SelectionKey* key) {
    int i = 0;

    TakeLock(selector->lock);

    SelectorWakeUp(selector);

    for (i = key->poll; i < selector->numPolls - 1; i++) {
        memcpy(&selector->polls[i], &selector->polls[i + 1], sizeof(aio4c_poll_t));
    }

    memset(&selector->polls[i], 0, sizeof(aio4c_poll_t));

    selector->numPolls--;

    for (i = key->index; i < selector->numKeys - 1; i++) {
        memcpy(&selector->keys[i], &selector->keys[i + 1], sizeof(SelectionKey));
    }

    memset(&selector->keys[i], 0, sizeof(SelectionKey));

    selector->numKeys--;

    ReleaseLock(selector->lock);
}

aio4c_size_t Select(Selector* selector) {
    aio4c_size_t nbPolls = 0;
    unsigned char dummy = 0;
    Thread* owner = ThreadSelf();

    if (owner != NULL) {
        owner->state = IDLE;
    }

    while ((nbPolls = poll(selector->polls, selector->numPolls, -1)) < 0) {
        if (errno != EINTR) {
            if (owner != NULL) {
                owner->state = RUNNING;
            }
            return 0;
        }
    }

    if (owner != NULL) {
        owner->state = RUNNING;
    }

    TakeLock(selector->lock);

    if (selector->polls[0].revents == POLLIN) {
        read(selector->pipe[AIO4C_PIPE_READ], &dummy, sizeof(unsigned char));
        selector->polls[0].revents = 0;
        nbPolls--;
    }

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

    if (pSelector != NULL && (selector = *pSelector) != NULL) {
        close(selector->pipe[AIO4C_PIPE_WRITE]);
        close(selector->pipe[AIO4C_PIPE_READ]);
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

    for (i = selector->curKey; i < selector->numKeys; i++) {
        if (selector->polls[selector->keys[i].poll].revents > 0) {
            selector->keys[i].result = selector->polls[selector->keys[i].poll].revents;
            *key = &selector->keys[i];
            selector->curKey++;
            break;
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
    Thread* parent = ThreadSelf();

    TakeLock(thread->lock);

    if (ThreadRunning(thread)) {
        if (force) {
            Log(parent, DEBUG, "cancelling thread %s", thread->name);
            aio4c_thread_cancel(thread->id);
            thread->state = STOPPED;
        } else {
            Log(parent, DEBUG, "stopping thread %s", thread->name);
            thread->state = STOPPED;
        }
    }

    ReleaseLock(thread->lock);

    return thread;
}

Thread* ThreadJoin(Thread* thread) {
    void* returnValue = NULL;
    Thread* parent = ThreadSelf();

    if (thread->state == JOINED || !thread->stateValid) {
        return thread;
    }

    Log(parent, DEBUG, "joining thread %s", thread->name);

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

