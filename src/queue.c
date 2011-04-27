/**
 * This file is part of Aio4c <http://aio4c.so>.
 *
 * Aio4c <http://aio4c.so> is free software: you
 * can  redistribute  it  and/or modify it under
 * the  terms  of the GNU General Public License
 * as published by the Free Software Foundation,
 * version 3 of the License.
 *
 * This  program is distributed in the hope that
 * it  will be useful, but WITHOUT ANY WARRANTY;
 * without   even   the   implied   warranty  of
 * MERCHANTABILITY  or  FITNESS FOR A PARTICULAR
 * PURPOSE.
 *
 * See  the  GNU General Public License for more
 * details.  You  should have received a copy of
 * the  GNU  General  Public  License along with
 * Aio4c    <http://aio4c.so>.   If   not,   see
 * <http://www.gnu.org/licenses/>.
 */
#include <aio4c/queue.h>

#include <aio4c/alloc.h>
#include <aio4c/condition.h>
#include <aio4c/error.h>
#include <aio4c/event.h>
#include <aio4c/list.h>
#include <aio4c/lock.h>
#include <aio4c/log.h>
#include <aio4c/stats.h>
#include <aio4c/types.h>

#ifndef AIO4C_WIN32
#include <errno.h>
#endif /* AIO4C_WIN32 */

#include <string.h>

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

    AIO4C_LIST_INITIALIZER(&queue->busy);
    AIO4C_LIST_INITIALIZER(&queue->free);
    queue->lock      = NewLock();
    queue->condition = NewCondition();
    queue->emptied   = false;
    queue->exit      = false;

    return queue;
}

aio4c_bool_t Dequeue(Queue* queue, QueueItem* item, aio4c_bool_t wait) {
    aio4c_bool_t dequeued = false;
    Node* node = NULL;

    TakeLock(queue->lock);

    if (queue->emptied) {
        queue->emptied = false;
        ReleaseLock(queue->lock);
        return false;
    }

    while (!queue->exit && wait && ListEmpty(&queue->busy)) {
        ProbeTimeStart(AIO4C_TIME_PROBE_IDLE);

        if (!WaitCondition(queue->condition, queue->lock)) {
            break;
        }

        ProbeTimeEnd(AIO4C_TIME_PROBE_IDLE);
    }

    if (!ListEmpty(&queue->busy)) {
        node = ListPop(&queue->busy);
        memcpy(item, node->data, sizeof(QueueItem));
        ListAddLast(&queue->free, node);
        queue->emptied = ListEmpty(&queue->busy);
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

static aio4c_bool_t _Enqueue(Queue* queue, QueueItem* item) {
    Node* node = NULL;

    if (queue == NULL || queue->lock == NULL) {
        return false;
    }

    TakeLock(queue->lock);

    if (queue->exit) {
        ReleaseLock(queue->lock);
        return false;
    }

    if (ListEmpty(&queue->free)) {
        ListAddLast(&queue->free, NewNode(_NewItem()));
    }

    node = ListPop(&queue->free);

    memcpy(node->data, item, sizeof(QueueItem));

    ListAddLast(&queue->busy, node);

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
    aio4c_bool_t removed = false;
    Node* i = NULL;
    Node* toRemove = NULL;

    TakeLock(queue->lock);

    NotifyCondition(queue->condition);

    for (i = queue->busy.first; i != NULL;) {
        if (removeCallback((QueueItem*)i->data, discriminant)) {
            toRemove = i;
            i = i->next;
            ListRemove(&queue->busy, toRemove);
            ListAddLast(&queue->free, toRemove);
            removed = true;
        } else {
            i = i->next;
        }
    }

    ReleaseLock(queue->lock);

    return removed;
}

void FreeQueue(Queue** queue) {
    Queue* pQueue = NULL;
    Node* i = NULL;

    if (queue != NULL && (pQueue = *queue) != NULL) {
        TakeLock(pQueue->lock);

        NotifyCondition(pQueue->condition);

        while (!ListEmpty(&pQueue->free)) {
            i = ListPop(&pQueue->free);
            aio4c_free(i->data);
            FreeNode(&i);
        }

        while (!ListEmpty(&pQueue->busy)) {
            i = ListPop(&pQueue->busy);
            aio4c_free(i->data);
            FreeNode(&i);
        }

        FreeCondition(&pQueue->condition);

        ReleaseLock(pQueue->lock);
        FreeLock(&pQueue->lock);

        aio4c_free(pQueue);
        *queue = NULL;
    }
}
