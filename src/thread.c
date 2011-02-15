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

#include <aio4c/types.h>

#include <stdlib.h>

Lock* NewLock(void) {
    Lock* pLock = NULL;

    if ((pLock = malloc(sizeof(Lock))) == NULL) {
        return NULL;
    }

    pLock->state = FREE;
    pLock->owner = NULL;

    if (aio4c_mutex_init(&pLock->mutex, NULL) != 0) {
        free(pLock);
        return NULL;
    }

    pLock->state = FREE;

    return pLock;
}

Lock* TakeLock(Thread* owner, Lock* lock) {
    if (owner != NULL && owner == lock->owner) {
        return NULL;
    }

    if (owner != NULL) {
        owner->state = BLOCKED;
    }

    if (aio4c_mutex_lock(&lock->mutex) != 0) {
        return NULL;
    }

    lock->owner = owner;
    lock->state = LOCKED;

    if (owner != NULL) {
        owner->state = RUNNING;
    }

    return lock;
}

Lock* ReleaseLock(Lock* lock) {
    if (aio4c_mutex_unlock(&lock->mutex) != 0) {
        return NULL;
    }

    lock->owner = NULL;
    lock->state = FREE;

    return NULL;
}

void FreeLock(Lock** lock) {
    Lock * pLock = NULL;

    if (lock != NULL && (pLock = *lock) != NULL) {
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

aio4c_bool_t WaitCondition(Thread* owner, Condition* condition, Lock* lock) {
    if ((condition->owner != NULL && condition->owner == owner) || ((lock->owner != NULL && lock->owner != owner) || lock->state != LOCKED)) {
        return false;
    }

    condition->owner = owner;

    if (owner != NULL) {
        owner->state = IDLE;
    }

    lock->state = FREE;
    lock->owner = NULL;

    if (aio4c_cond_wait(&condition->condition, &lock->mutex) != 0) {
        lock->owner = owner;
        lock->state = LOCKED;
        condition->owner = NULL;
        if (owner != NULL) {
            owner->state = RUNNING;
        }
        return false;
    }

    lock->owner = owner;
    lock->state = LOCKED;

    condition->owner = NULL;

    if (owner != NULL) {
        owner->state = RUNNING;
    }

    return true;
}

void NotifyCondition(Thread* notifier, Condition* condition, Lock* lock) {
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

static Thread* _addChild(Thread* parent, Thread* child) {
    if (parent == NULL) {
        return child;
    }

    if (parent->childrenCount >= parent->maxChildren) {
        if ((parent->children = realloc(parent->children, (parent->childrenCount + 1) * sizeof(Thread*))) == NULL) {
            return child;
        }

        parent->maxChildren = parent->childrenCount + 1;
    }

    parent->children[parent->childrenCount++] = child;
    child->parent = parent;

    return child;
}

static void _ThreadCleanup(Thread* thread) {
    ReleaseLock(thread->lock);

    thread->state = EXITED;

    if (thread->exit != NULL) {
        thread->exit(thread->arg);
    }
}

static Thread* _runThread(Thread* thread) {
    TakeLock(thread, thread->lock);

    if (thread->init != NULL) {
        thread->init(thread->arg);
    }

    aio4c_thread_cleanup_push((void(*)(void*))_ThreadCleanup, (void*)thread);

    while (thread->state != STOPPED) {
        ReleaseLock(thread->lock);

        if (!thread->run(thread->arg)) {
            break;
        }

        TakeLock(thread, thread->lock);
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
    _MainThread->children = NULL;
    _MainThread->childrenCount = 0;
    _MainThread->maxChildren = 0;
    _MainThread->parent = NULL;

    return _MainThread;
}

Thread* ThreadSelf(Thread* parent) {
    int i = 0;
    Thread* child = NULL;

    if (parent == NULL && _MainThread == NULL) {
        return NULL;
    }

    if (parent == NULL) {
        parent = _MainThread;
    }

    for (i = 0; i < parent->childrenCount; i++) {
        if (parent->children[i]->id == aio4c_thread_self()) {
            return parent->children[i];
        }
    }

    for (i = 0; i < parent->childrenCount; i++) {
        if ((child = ThreadSelf(parent->children[i])) != NULL) {
            return child;
        }
    }

    return NULL;
}

Thread* NewThread(Thread* parent, char* name, void (*init)(void*), aio4c_bool_t (*run)(void*), void (*exit)(void*), void* arg) {
    Thread* thread = NULL;

    if ((thread = malloc(sizeof(Thread))) == NULL) {
        return NULL;
    }

    thread->name = name;
    thread->state = STOPPED;
    thread->lock = NewLock();
    thread->children = NULL;
    thread->childrenCount = 0;
    thread->maxChildren = 0;
    thread->init = init;
    thread->run = run;
    thread->exit = exit;
    thread->arg = arg;
    thread->parent = parent;

    TakeLock(parent, thread->lock);

    if (aio4c_thread_create(&thread->id, NULL, (void*(*)(void*))_runThread, (void*)thread) != 0) {
        ReleaseLock(thread->lock);
        FreeLock(&thread->lock);
        free(thread);
        return NULL;
    }

    thread->state = RUNNING;

    _addChild(parent, thread);

    ReleaseLock(thread->lock);

    return thread;
}

Thread* ThreadCancel(Thread* parent, Thread* thread, aio4c_bool_t force) {
    if (thread->state == RUNNING || thread->state == BLOCKED || thread->state == IDLE) {
        if (force) {
            aio4c_thread_cancel(thread->id);
        } else {
            TakeLock(parent, thread->lock);
            thread->state = STOPPED;
            ReleaseLock(thread->lock);
        }
    }

    return thread;
}

Thread* ThreadJoin(Thread* parent, Thread* thread) {
    void* returnValue = NULL;
    int i = 0;

    if (thread->state != STOPPED && thread->state != EXITED) {
        ThreadCancel(parent, thread, false);
    }

    if (thread->state == JOINED) {
        return thread;
    }

    aio4c_thread_join(thread->id, &returnValue);

    aio4c_thread_detach(thread->id);

    thread->state = JOINED;

    TakeLock(parent, thread->parent->lock);

    for (i = 0; i < thread->parent->childrenCount; i++) {
        if (thread->parent->children[i] == thread) {
            break;
        }
    }

    for (; i < thread->parent->childrenCount - 1; i++) {
        thread->parent->children[i] = thread->parent->children[i + 1];
    }

    thread->parent->childrenCount--;
    thread->parent->children[thread->parent->childrenCount] = NULL;

    ReleaseLock(thread->lock);

    return thread;
}

void FreeThread(Thread* parent, Thread** thread) {
    Thread* pThread = NULL;
    int i = 0;

    if (thread != NULL && (pThread = *thread) != NULL) {
        if (pThread->state != JOINED && parent != NULL) {
            if (pThread->state != EXITED) {
                ThreadCancel(parent, pThread, true);
            }
            ThreadJoin(parent, pThread);
        }

        for (i=0; i<pThread->childrenCount; i++) {
            if (parent != NULL) {
                _addChild(parent, pThread->children[i]);
            } else {
                pThread->children[i]->parent = NULL;
            }
        }

        if (pThread->children != NULL) {
            free(pThread->children);
        }
        FreeLock(&pThread->lock);
        free(pThread);
        *thread = NULL;
    }
}

