/*
 * Copyright (c) 2011 blakawk
 *
 * This file is part of Aio4c <http://aio4c.so>.
 *
 * Aio4c <http://aio4c.so> is free software: you
 * can  redistribute  it  and/or modify it under
 * the  terms  of the GNU General Public License
 * as published by the Free Software Foundation,
 * version 3 of the License.
 *
 * Aio4c <http://aio4c.so> is distributed in the
 * hope  that it will be useful, but WITHOUT ANY
 * WARRANTY;  without  even the implied warranty
 * of   MERCHANTABILITY   or   FITNESS   FOR   A
 * PARTICULAR PURPOSE.
 *
 * See  the  GNU General Public License for more
 * details.  You  should have received a copy of
 * the  GNU  General  Public  License along with
 * Aio4c    <http://aio4c.so>.   If   not,   see
 * <http://www.gnu.org/licenses/>.
 */
/**
 * @file aio4c/queue.h
 * @brief Provides a FIFO implementation.
 *
 * @author blakawk
 */
#ifndef __AIO4C_QUEUE_H__
#define __AIO4C_QUEUE_H__

#include <aio4c/event.h>
#include <aio4c/list.h>
#include <aio4c/types.h>

/**
 * @enum QueueItemType
 * @brief The kind of QueueItem.
 */
typedef enum e_QueueItemType {
    AIO4C_QUEUE_ITEM_UNDEFINED = 0, /**< The QueueItem is not allocated. */
    AIO4C_QUEUE_ITEM_EVENT = 1,     /**< The QueueItem is an Event. */
    AIO4C_QUEUE_ITEM_DATA = 2,      /**< The QueueItem is a data pointer. */
    AIO4C_QUEUE_ITEM_TASK = 3,      /**< The QueueItem is a Task. */
    AIO4C_QUEUE_ITEM_EXIT = 4,      /**< The QueueItem is an exit request. */
    AIO4C_QUEUE_ITEM_MAX = 5        /**< Maximum number of QueueItem types. */
} QueueItemType;

/**
 * @def __AIO4C_CONNECTION_DEFINED__
 * @brief Defined when Connection type is defined.
 */
#ifndef __AIO4C_CONNECTION_DEFINED__
#define __AIO4C_CONNECTION_DEFINED__
typedef struct s_Connection Connection;
#endif /* __AIO4C_CONNECTION_DEFINED__ */

/**
 * @def __AIO4C_BUFFER_DEFINED__
 * @brief Defined when Buffer type is defined.
 */
#ifndef __AIO4C_BUFFER_DEFINED__
#define __AIO4C_BUFFER_DEFINED__
typedef struct s_Buffer Buffer;
#endif /* __AIO4C_BUFFER_DEFINED__ */

/**
 * @def __AIO4C_LOCK_DEFINED__
 * @brief Defined when Lock type is defined.
 */
#ifndef __AIO4C_LOCK_DEFINED__
#define __AIO4C_LOCK_DEFINED__
typedef struct s_Lock Lock;
#endif /* __AIO4C_LOCK_DEFINED__ */

/**
 * @def __AIO4C_CONDITION_DEFINED__
 * @brief Defined when Condition type is defined.
 */
#ifndef __AIO4C_CONDITION_DEFINED__
#define __AIO4C_CONDITION_DEFINED__
typedef struct s_Condition Condition;
#endif /* __AIO4C_CONDITION_DEFINED__ */

/**
 * @struct s_QueueEventItem
 * @brief Represents a QueueItem of type EVENT.
 *
 * It holds an Event to be delivered to a Thread from a specific EventSource.
 *
 * @see Event
 * @see EventSource
 */
/**
 * @def __AIO4C_QUEUE_EVENT_ITEM_DEFINED__
 * @brief Defined when QueueEventItem type is defined.
 */
#ifndef __AIO4C_QUEUE_EVENT_ITEM_DEFINED__
#define __AIO4C_QUEUE_EVENT_ITEM_DEFINED__
typedef struct s_QueueEventItem QueueEventItem;
#endif /* __AIO4C_QUEUE_EVENT_ITEM_DEFINED__ */

/**
 * @struct s_QueueTaskItem
 * @brief Represents a QueueItem of type TASK.
 *
 * It is mainly used by Worker. It holds a Buffer to be processed for a Connection.
 *
 * @see Worker
 */
/**
 * @def __AIO4C_QUEUE_TASK_ITEM_DEFINED__
 * @brief Defined when QueueTaskItem type is defined.
 */
#ifndef __AIO4C_QUEUE_TASK_ITEM_DEFINED__
#define __AIO4C_QUEUE_TASK_ITEM_DEFINED__
typedef struct s_QueueTaskItem QueueTaskItem;
#endif /* __AIO4C_QUEUE_TASK_ITEM_DEFINED__ */

/**
 * @union u_QueueItemData
 * @brief Represents the data held by a QueueItem.
 *
 * It is an union composed of either a QueueEventItem, a QueueTaskItem or a pointer
 * to any generic data structure according to the QueueItemType.
 */
/**
 * @def __AIO4C_QUEUE_ITEM_DATA_DEFINED__
 * @brief Defined when QueueItemData type is defined.
 */
#ifndef __AIO4C_QUEUE_ITEM_DATA_DEFINED__
#define __AIO4C_QUEUE_ITEM_DATA_DEFINED__
typedef union u_QueueItemData QueueItemData;
#endif /* __AIO4C_QUEUE_ITEM_DATA_DEFINED__ */

/**
 * @struct s_QueueItem
 * @brief An element of a Queue.
 *
 * It holds different data according to its QueueItemType.
 *
 * @see QueueItemData
 */
/**
 * @def __AIO4C_QUEUE_ITEM_DEFINED__
 * @brief Defined when QueueItem type is defined.
 */
#ifndef __AIO4C_QUEUE_ITEM_DEFINED__
#define __AIO4C_QUEUE_ITEM_DEFINED__
typedef struct s_QueueItem QueueItem;
#endif /* __AIO4C_QUEUE_ITEM_DEFINED__ */

/**
 * @struct s_Queue
 * @brief Represents a generic FIFO.
 *
 * One can enqueue QueueItems, another can dequeue them. When the Queue is empty,
 * the dequeue operation will block until one enqueue an item if required.
 */
/**
 * @def __AIO4C_QUEUE_DEFINED__
 * @brief Defined when Queue type is defined.
 */
#ifndef __AIO4C_QUEUE_DEFINED__
#define __AIO4C_QUEUE_DEFINED__
typedef struct s_Queue Queue;
#endif /* __AIO4C_QUEUE_DEFINED__ */

/**
 * @typedef QueueDiscriminant
 * @brief Represents a discriminant to determine if a QueueItem is to be removed.
 *
 * The QueueDiscriminant will be passed to the QueueRemoveCallback in order to
 * determine if a QueueItem is to be removed from a Queue.
 */
typedef struct s_QueueDiscriminant* QueueDiscriminant;

/**
 * @typedef QueueRemoveCallback
 * @brief Callback used to determine if a QueueItem is to be removed.
 */
typedef bool (*QueueRemoveCallback)(QueueItem*,QueueDiscriminant);

/**
 * @fn Queue* NewQueue(void)
 * @brief Allocates a Queue.
 *
 * After initialization, the Queue will be empty.
 *
 * @return
 *   Pointer to the newly allocated Queue.
 */
extern AIO4C_API Queue* NewQueue(void);

/**
 * @fn QueueItem* NewQueueItem(void)
 * @brief Allocates a QueueItem storage.
 *
 * The QueueItem will be of type UNDEFINED until it is filled in by Dequeue 
 *
 * @return
 *   Pointer to the newly allocated QueueItem.
 */
extern AIO4C_API QueueItem* NewQueueItem(void);

/**
 * @fn QueueItemType QueueItemGetType(QueueItem*)
 * @brief Gets the QueueItem type.
 *
 * @param item
 *   Pointer to the QueueItem to retrieve the QueueItemType from.
 * @return
 *   The QueueItemType of the QueueItem.
 */
extern AIO4C_API QueueItemType QueueItemGetType(QueueItem* item);

/**
 * @fn bool EnqueueDataItem(Queue*,void*)
 * @brief Enqueue an item of type DATA.
 *
 * @param queue
 *   Pointer to the Queue.
 * @param data
 *   Pointer to the data to be enqueued.
 * @return
 *   <code>true</code> if the item has been enqueued with success, else <code>false</code>.
 */
extern AIO4C_API bool EnqueueDataItem(Queue* queue, void* data);

/**
 * @fn void* QueueDataItemGet(QueueItem*)
 * @brief Gets the QueueItem held data pointer.
 *
 * @param item
 *   Pointer to the QueueItem.
 * @return
 *   The data pointer held by the QueueItem.
 */
extern AIO4C_API void* QueueDataItemGet(QueueItem* item);

/**
 * @fn bool EnqueueExitItem(Queue*)
 * @brief Enqueue an item of type EXIT.
 *
 * Theses item are generally used in order to terminate the Thread waiting on the Queue.
 *
 * @param queue
 *   Pointer to the Queue.
 * @return
 *   <code>true</code> if the item has been enqueued with success, else <code>false</code>.
 */
extern AIO4C_API bool EnqueueExitItem(Queue* queue);

/**
 * @fn bool EnqueueEventItem(Queue*,Event,EventSource)
 * @brief Enqueue an item of type EVENT.
 *
 * A QueueItem of type Event allows to deliver a specific Event from an EventSource.
 *
 * @param queue
 *   Pointer to the Queue.
 * @param type
 *   The type of Event to deliver.
 * @param source
 *   The source of the Event.
 * @return
 *   <code>true</code> if the item has been enqueued with success, else <code>false</code>.
 */
extern AIO4C_API bool EnqueueEventItem(Queue* queue, Event type, EventSource source);

/**
 * @fn Event QueueEventItemGetEvent(QueueItem*)
 * @brief Gets the Event associated to a QueueItem of type EVENT.
 *
 * @param item
 *   Pointer to the QueueItem.
 * @return
 *   The QueueItem Event type.
 */
extern AIO4C_API Event QueueEventItemGetEvent(QueueItem* item);

/**
 * @fn EventSoure QueueEventItemGetSource(QueueItem*)
 * @brief Gets the EventSource associated to a QueueItem of type EVENT.
 *
 * @param item
 *   Pointer to the QueueItem.
 * @return
 *   The QueueItem EventSource.
 */
extern AIO4C_API EventSource QueueEventItemGetSource(QueueItem* item);

/**
 * @fn bool EnqueueTaskItem(Queue*,Event,Connection*,Buffer*)
 * @brief Enqueue an item of type TASK.
 *
 * A QueueItem of type TASK allows to send a Task to a Worker in order to process
 * an Event from a Connection in a Buffer.
 *
 * @param queue
 *   Pointer to the Queue.
 * @param event
 *   The type of Event to deliver.
 * @param connection
 *   The Connection associated with the Buffer and the Event.
 * @param buffer
 *   The Buffer to retrieve data from.
 * @return
 *   <code>true</code> if the item has been enqueued with success, else <code>false</code>.
 */
extern AIO4C_API bool EnqueueTaskItem(Queue* queue, Event event, Connection* connection, Buffer* buffer);

/**
 * @fn Event QueueTaskItemGetEvent(QueueItem*)
 * @brief Gets the Event associated to a QueueItem of type TASK.
 *
 * @param item
 *   Pointer to the QueueItem.
 * @return
 *   The QueueItem Event type.
 */
extern AIO4C_API Event QueueTaskItemGetEvent(QueueItem* item);

/**
 * @fn Connection* QueueTaskItemGetConnection(QueueItem*)
 * @brief Gets the Connection associated to a QueueItem of type TASK.
 *
 * @param item
 *   Pointer to the QueueItem.
 * @return
 *   The QueueItem Connection pointer.
 */
extern AIO4C_API Connection* QueueTaskItemGetConnection(QueueItem* item);

/**
 * @fn Buffer* QueueTaskItemGetBuffer(QueueItem*)
 * @brief Gets the Buffer associated to a QueueItem of type TASK.
 *
 * @param item
 *   Pointer to the QueueItem.
 * @return
 *   The QueueItem Buffer poitner.
 */
extern AIO4C_API Buffer* QueueTaskItemGetBuffer(QueueItem* item);

/**
 * @fn bool Dequeue(Queue*,QueueItem*,bool)
 * @brief Dequeue an item from a Queue.
 *
 * In order to remove an item from a Queue, the queue should be not empty.
 * If the Queue is empty, then according to the wait parameter, the call blocks or not.
 * If the call blocks, it will be unblocked when an item is enqueued to the queue.
 *
 * The item parameter must not be NULL or undefined behaviour will occur (memory corruption).
 *
 * @param queue
 *   Pointer to the Queue to remove the item from.
 * @param item
 *   Pointer to the item storage.
 * @param wait
 *   <code>true</code> if the call should wait for an item to be enqueued when the queue is empty, <code>false</code> if the call should
 *   return immediately.
 * @return
 *   <code>true</code> if an item has been dequeued, <code>false</code> if not.
 */
extern AIO4C_API bool Dequeue(Queue* queue, QueueItem* item, bool wait);

/**
 * @fn bool RemoveAll(Queue*,QueueRemoveCallback,QueueDiscriminant)
 * @brief Removes several items from a Queue.
 *
 * For each item present in the Queue, the QueueRemoveCallback is called with the discriminant as parameter. If the callback
 * return true, then the item is removed from the queue, else it is kept.
 *
 * @param queue
 *   Pointer to the Queue.
 * @param removeCallback
 *   The callback used to determine if an item is to be removed.
 * @param discriminant
 *   The parameter used by the callback to determine if an item is to be removed.
 * @return
 *   <code>true</code> if at least one item has been removed from the Queue, else <code>false</code>.
 */
extern AIO4C_API bool RemoveAll(Queue* queue, QueueRemoveCallback removeCallback, QueueDiscriminant discriminant);

/**
 * @fn void FreeQueueItem(QueueItem**)
 * @brief Frees a QueueItem storage.
 *
 * Frees the memory used by a QueueItem.
 * The pointed pointer will be set to NULL once the free is performed in order to disallow further access.
 *
 * @param item
 *   Pointer to a pointer to a QueueItem.
 */
extern AIO4C_API void FreeQueueItem(QueueItem** item);

/**
 * @fn void FreeQueue(Queue**)
 * @brief Frees a Queue.
 *
 * Frees all item still present in the Queue and then frees the Queue.
 * The pointed pointer is set to NULL afterwards.
 *
 * @param queue
 *   A pointer to a Queue pointer.
 */
extern AIO4C_API void FreeQueue(Queue** queue);

#endif /* __AIO4C_QUEUE_H__ */
