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
#ifndef __AIO4C_SELECTOR_H__
#define __AIO4C_SELECTOR_H__

#include <aio4c/list.h>
#include <aio4c/types.h>

#ifndef AIO4C_WIN32
#include <poll.h>
#endif /* AIO4C_WIN32 */

#ifndef __AIO4C_LOCK_DEFINED__
#define __AIO4C_LOCK_DEFINED__
typedef struct s_Lock Lock;
#endif /* __AIO4C_LOCK_DEFINED__ */

typedef enum e_SelectionOperation {
#ifdef AIO4C_HAVE_POLL
#ifndef AIO4C_WIN32
    AIO4C_OP_READ = POLLIN,
    AIO4C_OP_WRITE = POLLOUT
#else /* AIO4C_WIN32 */
    AIO4C_OP_READ = POLLRDNORM,
    AIO4C_OP_WRITE = POLLWRNORM
#endif /* AIO4C_WIN32 */
#else /* AIO4C_HAVE_POLL */
    AIO4C_OP_READ  = 0x01,
    AIO4C_OP_WRITE = 0x02,
    AIO4C_OP_ERROR = 0x04
#endif /* AIO4C_HAVE_POLL */
} SelectionOperation;

typedef struct s_SelectionKey {
    SelectionOperation operation;
    Node*              node;
    aio4c_socket_t     fd;
    void*              attachment;
    int                poll;
    int                count;
    int                curCount;
    short              result;
} SelectionKey;

#ifndef AIO4C_HAVE_POLL
typedef struct s_Poll {
    aio4c_socket_t fd;
    short          events;
    short          revents;
} Poll;
#endif /* AIO4C_HAVE_POLL */

typedef struct s_Selector {
    aio4c_pipe_t    pipe;
    List            busyKeys;
    List            freeKeys;
    SelectionKey*   curKey;
#ifdef AIO4C_HAVE_POLL
    aio4c_poll_t*   polls;
#else /* AIO4C_HAVE_POLL */
    Poll*           polls;
#endif /* AIO4C_HAVE_POLL */
    int             numPolls;
    int             maxPolls;
    Lock*           lock;
#ifndef AIO4C_HAVE_PIPE
    int             port;
#endif /* AIO4C_HAVE_PIPE */
} Selector;

extern AIO4C_API Selector* NewSelector(void);

extern AIO4C_API SelectionKey* Register(Selector* selector, SelectionOperation operation, aio4c_socket_t fd, void* attachment);

extern AIO4C_API void Unregister(Selector* selector, SelectionKey* key, aio4c_bool_t unregisterAll, aio4c_bool_t* isLastRegistration);

#define Select(selector) \
    _Select(__FILE__, __LINE__, selector)
extern AIO4C_API aio4c_size_t _Select(char* file, int line, Selector* selector);

#define SelectorWakeUp(selector) \
    _SelectorWakeUp(__FILE__, __LINE__, selector)
extern AIO4C_API void _SelectorWakeUp(char* file, int line, Selector* selector);

extern AIO4C_API aio4c_bool_t SelectionKeyReady(Selector* selector, SelectionKey** key);

extern AIO4C_API void FreeSelector(Selector** selector);

#endif
