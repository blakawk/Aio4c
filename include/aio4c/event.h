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
 * @file aio4c/event.h
 * @brief Provides Connection Events management utilities.
 *
 * Each Event is related to a Connection, and allows to call Event handlers.
 *
 * @author blakawk
 */
#ifndef __AIO4C_EVENT_H__
#define __AIO4C_EVENT_H__

#include <aio4c/types.h>

/**
 * @enum Event
 * @brief Represents the Event handled by a Connection.
 */
typedef enum e_EventType {
    AIO4C_INIT_EVENT = 0,          /**< Event received when Connection is initialized. */
    AIO4C_CONNECTING_EVENT = 1,    /**< Event received when a Connection is establishing (only for Client Connections). */
    AIO4C_CONNECTED_EVENT = 2,     /**< Event received when a Connection is established (Client) or accepted (Server). */
    AIO4C_READ_EVENT = 3,          /**< Event received when data is read from a Connection to a Buffer. */
    AIO4C_INBOUND_DATA_EVENT = 4,  /**< Event received when a Buffer is to be processed from a Connection. */
    AIO4C_WRITE_EVENT = 5,         /**< Event received to write data from a Buffer to a Connection. */
    AIO4C_OUTBOUND_DATA_EVENT = 6, /**< Event received when data is available to be sent on a Connection. */
    AIO4C_PENDING_CLOSE_EVENT = 7, /**< Event received when a Connection is about to be closed. */
    AIO4C_CLOSE_EVENT = 8,         /**< Event received when a Connection is closed. */
    AIO4C_FREE_EVENT = 9,          /**< Event received when a Connection is freed. */
    AIO4C_EVENTS_COUNT = 10        /**< The number of Events. */
} Event;

/**
 * @typedef EventSource
 * @brief Pointer to the source of an Event.
 *
 * The pointed data is passed through EventCallback when an Event is handled.
 */
typedef struct s_EventSource* EventSource;

/**
 * @typedef EventData
 * @brief Pointer to additionnal data to pass to the callback of an Event.
 */
typedef struct s_EventData* EventData;

/**
 * @typedef EventCallback
 * @brief Callback called when an Event is handled.
 *
 * It takes three parameters:
 *   - Event: the handled Event,
 *   - EventSource: the Event source,
 *   - EventData: the data associated with the Event.
 */
typedef void (*EventCallback)(Event,EventSource,EventData);

/**
 * @struct s_EventHandler
 * @brief Represents a callback that handles Events for a Connection.
 */
/**
 * @def __AIO4C_EVENT_HANDLER_DEFINED__
 * @brief Defined if EventHandler type has been defined.
 */
#ifndef __AIO4C_EVENT_HANDLER_DEFINED__
#define __AIO4C_EVENT_HANDLER_DEFINED__
typedef struct s_EventHandler EventHandler;
#endif /* __AIO4C_EVENT_HANDLER_DEFINED__ */

/**
 * @struct s_EventQueue
 * @brief Represents a queue of Event.
 *
 * This queue contains all EventHandlers to call when an Event is handled.
 */
/**
 * @def __AIO4C_EVENT_QUEUE_DEFINED__
 * @brief Defined if EventQueue type has been defined.
 */
#ifndef __AIO4C_EVENT_QUEUE_DEFINED__
#define __AIO4C_EVENT_QUEUE_DEFINED__
typedef struct s_EventQueue EventQueue;
#endif /* __AIO4C_EVENT_QUEUE_DEFINED__ */

/**
 * @fn EventQueue* NewEventQueue(void)
 * @brief Allocates an EventQueue.
 *
 * @return
 *   A pointer to the EventQueue.
 */
extern AIO4C_API EventQueue* NewEventQueue(void);

/**
 * @fn EventHandler* NewEventHandler(Event,EventCallback,EventData,aio4c_bool_t)
 * @brief Allocates an EventHandler.
 *
 * @param event
 *   The Event that will be handled by this EventHandler.
 * @param callback
 *   The EventCallback that will be called when the Event is handled.
 * @param data
 *   The additional data to be passed to the EventCallback.
 * @param once
 *   If <code>true</code>, the handler will be called only once and removed after the Event has been handled.
 * @return
 *   A pointer to the EventHandler.
 */
extern AIO4C_API EventHandler* NewEventHandler(Event event, EventCallback callback, EventData data, aio4c_bool_t once);

/**
 * @fn EventQueue* EventHandlerAdd(EventQueue*,EventHandler*)
 * @brief Add an EventHandler to an EventQueue.
 *
 * @param queue
 *   Pointer to the queue to add the handler in.
 * @param handler
 *   Pointer to the handler to add to the queue.
 * @return
 *   A pointer to the EventQueue where the EventHandler has been added.
 */
extern AIO4C_API EventQueue* EventHandlerAdd(EventQueue* queue, EventHandler* handler);

/**
 * @fn void EventHandle(EventQueue*,Event,EventSource)
 * @brief Handle an Event.
 *
 * When handling an Event, all the EventHandlers from the EventQueue that are known to handle
 * the Event event will be called with the EventSource as a 3rd parameter.
 * If an EventHandler is set to be called only once, it will then be removed from the EventQueue.
 *
 * @param queue
 *   A pointer to the EventQueue.
 * @param event
 *   The Event to handle.
 * @param source
 *   The EventSource of the Event.
 */
extern AIO4C_API void EventHandle(EventQueue* queue, Event event, EventSource source);

/**
 * @fn void EventHandlerRemove(EventQueue*,Event,EventCallback)
 * @brief Remove an EventHandler from an EventQueue.
 *
 * The EventHandler is free'd after being removed from the queue.
 *
 * @param queue
 *   The EventQueue to remove the handler from.
 * @param event
 *   The handled event of the EventHandler to remove.
 * @param callback
 *   The callback of the EventHandler to remove.
 */
extern AIO4C_API void EventHandlerRemove(EventQueue* queue, Event event, EventCallback callback);

/**
 * @fn void CopyEventQueue(EventQueue*,EventQueue*,EventData)
 * @brief Copy an EventQueue.
 *
 * Copy all EventHandlers from src to dst, changing the EventData with data if it is not NULL.
 *
 * @param dst
 *   The EventQueue to copy EventHandlers to.
 * @param src
 *   The EventQueue to copy EventHandlers from.
 * @param data
 *   If not NULL, this data will be used for the EventHandlers copy.
 */
extern AIO4C_API void CopyEventQueue(EventQueue* dst, EventQueue* src, EventData data);

/**
 * @fn void FreeEventQueue(EventQueue**)
 * @brief Frees an EventQueue.
 *
 * If the EventQueue is not empty, then the EventHandlers will be freed before freeing the queue.
 *
 * @param queue
 *   A pointer to a pointer to an EventQueue. Will be set to NULL once the queue is freed.
 */
extern AIO4C_API void FreeEventQueue(EventQueue** queue);

/**
 * @fn void FreeEventHandler(EventHandler**)
 * @brief Frees an EventHandler.
 *
 * Once the EventHandler is freed, the pointed pointer is set to NULL.
 *
 * @param handler
 *   A pointer to an EventHandler pointer.
 */
extern AIO4C_API void FreeEventHandler(EventHandler** handler);

#endif /* __AIO4C_EVENT_H__ */
