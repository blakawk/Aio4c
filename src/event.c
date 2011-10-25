/**
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
#include <aio4c/event.h>

#include <aio4c/alloc.h>
#include <aio4c/error.h>
#include <aio4c/list.h>
#include <aio4c/log.h>

#ifndef AIO4C_WIN32
#include <errno.h>
#endif /* AIO4C_WIN32 */

#include <stdlib.h>
#include <string.h>

struct s_EventHandler {
    Event           event;
    aio4c_bool_t    once;
    EventCallback   callback;
    EventData       data;
};

struct s_EventQueue {
    List handlers[AIO4C_EVENTS_COUNT];
};

EventQueue* NewEventQueue(void) {
    EventQueue* eventQueue = NULL;
    ErrorCode code = AIO4C_ERROR_CODE_INITIALIZER;

    if ((eventQueue = aio4c_malloc(sizeof(EventQueue))) == NULL) {
#ifndef AIO4C_WIN32
        code.error = errno;
#else /* AIO4C_WIN32 */
        code.source = AIO4C_ERRNO_SOURCE_SYS;
#endif /* AIO4C_WIN32 */
        code.size = sizeof(EventQueue);
        code.type = "EventQueue";
        Raise(AIO4C_LOG_LEVEL_ERROR, AIO4C_ALLOC_ERROR_TYPE, AIO4C_ALLOC_ERROR, &code);
        return NULL;
    }

    return eventQueue;
}

EventHandler* NewEventHandler(Event event, EventCallback callback, EventData data, aio4c_bool_t once) {
    EventHandler* eventHandler = NULL;
    ErrorCode code = AIO4C_ERROR_CODE_INITIALIZER;

    if ((eventHandler = aio4c_malloc(sizeof(EventHandler))) == NULL) {
#ifndef AIO4C_WIN32
        code.error = errno;
#else /* AIO4C_WIN32 */
        code.source = AIO4C_ERRNO_SOURCE_SYS;
#endif /* AIO4C_WIN32 */
        code.size = sizeof(EventHandler);
        code.type = "EventHandler";
        Raise(AIO4C_LOG_LEVEL_ERROR, AIO4C_ALLOC_ERROR_TYPE, AIO4C_ALLOC_ERROR, &code);
        return NULL;
    }

    eventHandler->event = event;
    eventHandler->callback = callback;
    eventHandler->data = data;
    eventHandler->once = once;

    return eventHandler;
}

EventQueue* EventHandlerAdd(EventQueue* queue, EventHandler* handler) {
    ListAddLast(&queue->handlers[handler->event], NewNode(handler));
    return queue;
}

void EventHandle(EventQueue* queue, Event event, EventSource source) {
    Node* i = NULL;
    Node* j = NULL;
    EventHandler* handler = NULL;

    for (i = queue->handlers[event].first; i != NULL;) {
        handler = (EventHandler*)i->data;
        handler->callback(event, source, handler->data);
        if (handler->once) {
            j = i->next;
            ListRemove(&queue->handlers[event], i);
            FreeEventHandler(&handler);
            FreeNode(&i);
            i = j;
        } else {
            i = i->next;
        }
    }
}

void EventHandlerRemove(EventQueue* queue, Event event, EventCallback callback) {
    Node* i = NULL;
    Node* j = NULL;
    EventHandler* _handler = NULL;

    for (i = queue->handlers[event].first; i != NULL;) {
        _handler = (EventHandler*)i->data;
        if (_handler->callback == callback) {
            j = i->next;
            ListRemove(&queue->handlers[event], i);
            FreeEventHandler(&_handler);
            FreeNode(&i);
            i = j;
        } else {
            i = i->next;
        }
    }
}

void CopyEventQueue(EventQueue* dst, EventQueue* src, EventData data) {
    int i = 0;
    Node* j = NULL;
    EventHandler* handler = NULL;

    for (i = 0; i < AIO4C_EVENTS_COUNT; i++) {
        for (j = src->handlers[i].first; j != NULL; j = j->next) {
            handler = (EventHandler*)j->data;
            if (data != NULL) {
                ListAddLast(&dst->handlers[i], NewNode(NewEventHandler(handler->event, handler->callback, data, handler->once)));
            } else {
                ListAddLast(&dst->handlers[i], NewNode(NewEventHandler(handler->event, handler->callback, handler->data, handler->once)));
            }
        }
    }
}

void FreeEventQueue(EventQueue** queue) {
    EventQueue* pQueue = NULL;
    int i = 0;
    Node* j = NULL;
    EventHandler* handler = NULL;

    if (queue != NULL && ((pQueue = *queue) != NULL)) {
        for (i=0; i<AIO4C_EVENTS_COUNT; i++) {
            while ((j = ListPop(&pQueue->handlers[i])) != NULL) {
                handler = (EventHandler*)j->data;
                FreeEventHandler(&handler);
                FreeNode(&j);
            }
        }

        aio4c_free(pQueue);
        *queue = NULL;
    }
}

void FreeEventHandler(EventHandler** handler) {
    EventHandler* pHandler = NULL;

    if (handler != NULL && ((pHandler = *handler) != NULL)) {
        aio4c_free(pHandler);
        *handler = NULL;
    }
}

