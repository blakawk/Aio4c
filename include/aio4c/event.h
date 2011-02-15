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
#ifndef __AIO4C_EVENT_H__
#define __AIO4C_EVENT_H__

#include <aio4c/types.h>

#define AIO_EVENTS_COUNT 6

typedef enum e_EventType {
    INIT_EVENT = 0,
    CONNECTING_EVENT = 1,
    CONNECTED_EVENT = 2,
    READ_EVENT = 3,
    WRITE_EVENT = 4,
    CLOSE_EVENT = 5
} Event;

typedef struct s_EventHandler {
    Event           event;
    aio4c_bool_t    once;
    void          (*handler)(Event,void*,void*);
    void*           arg;
} EventHandler;

#define aio4c_event_handler(handler) \
    (void(*)(Event,void*,void*))handler

#define aio4c_event_handler_arg(arg) \
    (void*)arg

typedef struct s_EventQueue {
    aio4c_size_t handlersCount[AIO_EVENTS_COUNT];
    aio4c_size_t maxHandlers[AIO_EVENTS_COUNT];
    EventHandler** eventHandlers[AIO_EVENTS_COUNT];
} EventQueue;

extern EventQueue* NewEventQueue(void);

extern EventHandler* NewEventHandler(Event event, void (*handler)(Event,void*,void*), void* arg, aio4c_bool_t once);

extern EventQueue* EventHandlerAdd(EventQueue* queue, EventHandler* handler);

extern void EventHandle(EventQueue* queue, Event event, void* source);

extern void EventHandlerRemove(EventQueue* queue, EventHandler* handler);

extern void FreeEventQueue(EventQueue** queue);

extern void FreeEventHandler(EventHandler** handler);

#endif
