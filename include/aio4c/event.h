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

#include <aio4c/list.h>
#include <aio4c/types.h>

typedef enum e_EventType {
    AIO4C_INIT_EVENT,
    AIO4C_CONNECTING_EVENT,
    AIO4C_CONNECTED_EVENT,
    AIO4C_READ_EVENT,
    AIO4C_INBOUND_DATA_EVENT,
    AIO4C_WRITE_EVENT,
    AIO4C_OUTBOUND_DATA_EVENT,
    AIO4C_PENDING_CLOSE_EVENT,
    AIO4C_CLOSE_EVENT,
    AIO4C_FREE_EVENT,
    AIO4C_EVENTS_COUNT
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
    List           handlers[AIO4C_EVENTS_COUNT];
} EventQueue;

extern EventQueue* NewEventQueue(void);

extern EventHandler* NewEventHandler(Event event, void (*handler)(Event,void*,void*), void* arg, aio4c_bool_t once);

extern EventQueue* EventHandlerAdd(EventQueue* queue, EventHandler* handler);

extern void EventHandle(EventQueue* queue, Event event, void* source);

extern void EventHandlerRemove(EventQueue* queue, Event event, void (*handler)(Event,void*,void*));

extern void CopyEventQueue(EventQueue* dst, EventQueue* src, void* arg);

extern void FreeEventQueue(EventQueue** queue);

extern void FreeEventHandler(EventHandler** handler);

#endif
