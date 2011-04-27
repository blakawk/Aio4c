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
#ifndef __AIO4C_QUEUE_H__
#define __AIO4C_QUEUE_H__

#include <aio4c/event.h>
#include <aio4c/list.h>
#include <aio4c/types.h>

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

#define aio4c_remove_callback(callback) \
    (aio4c_bool_t(*)(QueueItem*,void*))callback

#define aio4c_remove_discriminant(discriminant) \
    (void*)discriminant

#ifndef __AIO4C_CONNECTION_DEFINED__
#define __AIO4C_CONNECTION_DEFINED__
typedef struct s_Connection Connection;
#endif /* __AIO4C_CONNECTION_DEFINED__ */

#ifndef __AIO4C_BUFFER_DEFINED__
#define __AIO4C_BUFFER_DEFINED__
typedef struct s_Buffer Buffer;
#endif /* __AIO4C_BUFFER_DEFINED__ */

#ifndef __AIO4C_LOCK_DEFINED__
#define __AIO4C_LOCK_DEFINED__
typedef struct s_Lock Lock;
#endif /* __AIO4C_LOCK_DEFINED__ */

#ifndef __AIO4C_CONDITION_DEFINED__
#define __AIO4C_CONDITION_DEFINED__
typedef struct s_Condition Condition;
#endif /* __AIO4C_CONDITION_DEFINED__ */

typedef struct s_QueueTaskItem {
    Event       event;
    Connection* connection;
    Buffer*     buffer;
} QueueTaskItem;

typedef union u_QueueItemData {
    QueueTaskItem  task;
    QueueEventItem event;
    void*          data;
} QueueItemData;

typedef struct s_QueueItem {
    QueueItemType type;
    QueueItemData content;
} QueueItem;

typedef struct s_Queue {
    List         busy;
    List         free;
    Condition*   condition;
    Lock*        lock;
    aio4c_bool_t exit;
    aio4c_bool_t emptied;
} Queue;

extern AIO4C_API Queue* NewQueue(void);

extern AIO4C_API aio4c_bool_t EnqueueDataItem(Queue* queue, void* data);

extern AIO4C_API aio4c_bool_t EnqueueExitItem(Queue* queue);

extern AIO4C_API aio4c_bool_t EnqueueEventItem(Queue* queue, Event type, void* source);

extern AIO4C_API aio4c_bool_t EnqueueTaskItem(Queue* queue, Event event, Connection* connection, Buffer* buffer);

extern AIO4C_API aio4c_bool_t Dequeue(Queue* queue, QueueItem* item, aio4c_bool_t wait);

extern AIO4C_API aio4c_bool_t RemoveAll(Queue* queue, aio4c_bool_t (*removeCallback)(QueueItem*,void*), void* discriminant);

extern AIO4C_API void FreeQueue(Queue** queue);

#endif
