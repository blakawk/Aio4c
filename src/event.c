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
#include <aio4c/event.h>

#include <aio4c/alloc.h>
#include <aio4c/connection.h>

#include <stdlib.h>
#include <string.h>

EventQueue* NewEventQueue(void) {
    EventQueue* eventQueue = NULL;
    int i = 0;

    if ((eventQueue = aio4c_malloc(sizeof(EventQueue))) == NULL) {
        return NULL;
    }

    memset(eventQueue->eventHandlers, 0, AIO4C_EVENTS_COUNT * sizeof(EventHandler*));
    memset(eventQueue->handlersCount, 0, AIO4C_EVENTS_COUNT * sizeof(aio4c_size_t));
    memset(eventQueue->maxHandlers, 0, AIO4C_EVENTS_COUNT * sizeof(aio4c_size_t));

    for (i=0; i<AIO4C_EVENTS_COUNT; i++) {
        if ((eventQueue->eventHandlers[i] = aio4c_malloc(sizeof(EventHandler*))) == NULL) {
            break;
        }

        eventQueue->eventHandlers[i][0] = NULL;
        eventQueue->maxHandlers[i]++;
    }

    if (i < AIO4C_EVENTS_COUNT) {
        for (; i>=0; i--) {
            aio4c_free(eventQueue->eventHandlers[i]);
        }
        aio4c_free(eventQueue);
        eventQueue = NULL;
    }

    return eventQueue;
}

EventHandler* NewEventHandler(Event event, void (*handler)(Event,void*,void*), void* arg, aio4c_bool_t once) {
    EventHandler* eventHandler = NULL;

    if ((eventHandler = aio4c_malloc(sizeof(EventHandler))) == NULL) {
        return NULL;
    }

    eventHandler->event = event;
    eventHandler->handler = handler;
    eventHandler->arg = arg;
    eventHandler->once = once;

    return eventHandler;
}

EventQueue* EventHandlerAdd(EventQueue* queue, EventHandler* handler) {
    int i = 0;
    int handlerIndex = 0;

    if (queue->maxHandlers[handler->event] > queue->handlersCount[handler->event]) {
        while (i<queue->maxHandlers[handler->event] && queue->eventHandlers[handler->event][i++] != NULL);
        handlerIndex = i - 1;
    } else {
        if ((queue->eventHandlers[handler->event] = aio4c_realloc(queue->eventHandlers[handler->event], (queue->handlersCount[handler->event] + 1) * sizeof(EventHandler*))) == NULL) {
            FreeEventQueue(&queue);
            return NULL;
        }

        queue->maxHandlers[handler->event]++;
        handlerIndex = queue->handlersCount[handler->event];
    }

    queue->eventHandlers[handler->event][handlerIndex] = handler;
    queue->handlersCount[handler->event]++;

    return queue;
}

void EventHandle(EventQueue* queue, Event event, void* source) {
    int i = 0;

    for (i=0; i<queue->maxHandlers[event]; i++) {
        if (queue->eventHandlers[event][i] != NULL) {
            queue->eventHandlers[event][i]->handler(event, source, queue->eventHandlers[event][i]->arg);
            if (queue->eventHandlers[event][i]->once) {
                FreeEventHandler(&queue->eventHandlers[event][i]);
                queue->handlersCount[event]--;
            }
        }
    }
}

void EventHandlerRemove(EventQueue* queue, Event event, void (*handler)(Event,void*,void*)) {
    int i = 0;

    for (i=0; i<queue->maxHandlers[event]; i++) {
        if (queue->eventHandlers[event][i]->handler == handler) {
            FreeEventHandler(&queue->eventHandlers[event][i]);
            queue->handlersCount[event]--;
        }
    }
}

void CopyEventQueue(EventQueue* dst, EventQueue* src, void* arg) {
    int i = 0, j = 0;
    EventHandler* handler = NULL;

    for (i = 0; i < AIO4C_EVENTS_COUNT; i++) {
        for (j = 0; j < src->handlersCount[i]; j++) {
            handler = src->eventHandlers[i][j];
            if (handler != NULL) {
                if (arg != NULL) {
                    EventHandlerAdd(dst, NewEventHandler(handler->event, handler->handler, arg, handler->once));
                } else {
                    EventHandlerAdd(dst, NewEventHandler(handler->event, handler->handler, handler->arg, handler->once));
                }
            }
        }
    }
}

void FreeEventQueue(EventQueue** queue) {
    EventQueue* pQueue = NULL;
    int i = 0, j = 0;

    if (queue != NULL && ((pQueue = *queue) != NULL)) {
        for (i=0; i<AIO4C_EVENTS_COUNT; i++) {
            if (pQueue->eventHandlers[i] != NULL) {
                for (j=0; j<pQueue->maxHandlers[i]; j++) {
                    if (pQueue->eventHandlers[i][j] != NULL) {
                        FreeEventHandler(&pQueue->eventHandlers[i][j]);
                    }
                }
                aio4c_free(pQueue->eventHandlers[i]);
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

