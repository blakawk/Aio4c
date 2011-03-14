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
#include <aio4c/selector.h>

#include <aio4c/alloc.h>
#include <aio4c/error.h>
#include <aio4c/lock.h>
#include <aio4c/thread.h>

#ifndef AIO4C_WIN32
#include <arpa/inet.h>
#include <errno.h>
#include <fcntl.h>
#include <netinet/in.h>
#include <netinet/ip.h>
#include <poll.h>
#include <sys/select.h>
#include <sys/socket.h>
#include <sys/time.h>
#include <sys/types.h>
#include <unistd.h>
#else /* AIO4C_WIN32 */
#include <winbase.h>
#include <winsock2.h>
#endif /* AIO4C_WIN32 */

#include <string.h>

#ifndef AIO4C_HAVE_PIPE
static int _selectorNextPort = 8000;
#endif /* AIO4C_HAVE_PIPE */

Selector* NewSelector(void) {
    Selector* selector = NULL;
    ErrorCode code = AIO4C_ERROR_CODE_INITIALIZER;

    if ((selector = aio4c_malloc(sizeof(Selector))) == NULL) {
#ifndef AIO4C_WIN32
        code.error = errno;
#else /* AIO4C_WIN32 */
        code.source = AIO4C_ERRNO_SOURCE_SYS;
#endif /* AIO4C_WIN32 */
        code.size = sizeof(Selector);
        code.type = "Selector";
        Raise(AIO4C_LOG_LEVEL_ERROR, AIO4C_ALLOC_ERROR_TYPE, AIO4C_ALLOC_ERROR, &code);
        return NULL;
    }

#ifdef AIO4C_HAVE_POLL
    if ((selector->polls = aio4c_malloc(sizeof(aio4c_poll_t))) == NULL) {
#else /* AIO4C_HAVE_POLL */
    if ((selector->polls = aio4c_malloc(sizeof(Poll))) == NULL) {
#endif /* AIO4C_HAVE_POLL */
#ifndef AIO4C_WIN32
        code.error = errno;
#else /* AIO4C_WIN32 */
        code.source = AIO4C_ERRNO_SOURCE_SYS;
#endif /* AIO4C_WIN32 */
#ifdef AIO4C_HAVE_POLL
        code.size = sizeof(aio4c_poll_t);
        code.type = "aio4c_poll_t";
#else /* AIO4C_HAVE_POLL */
        code.size = sizeof(Poll);
        code.type = "Poll";
#endif /* AIO4C_HAVE_POLL */
        Raise(AIO4C_LOG_LEVEL_ERROR, AIO4C_ALLOC_ERROR_TYPE, AIO4C_ALLOC_ERROR, &code);
        aio4c_free(selector);
        return NULL;
    }

#ifdef AIO4C_HAVE_PIPE
    if (pipe(selector->pipe) != 0) {
        code.error = errno;
#else /* AIO4C_HAVE_PIPE */
#ifndef AIO4C_WIN32
    if ((selector->pipe[AIO4C_PIPE_READ] = socket(AF_INET, SOCK_DGRAM, 0)) == -1) {
        code.error = errno;
#else /* AIO4C_WIN32 */
    if ((selector->pipe[AIO4C_PIPE_READ] = socket(AF_INET, SOCK_DGRAM, 0)) == INVALID_SOCKET) {
        code.source = AIO4C_ERRNO_SOURCE_WSA;
#endif /* AIO4C_WIN32 */
#endif /* AIO4C_HAVE_PIPE */
        code.selector = selector;
        Raise(AIO4C_LOG_LEVEL_ERROR, AIO4C_THREAD_SELECTOR_ERROR_TYPE, AIO4C_THREAD_SELECTOR_INIT_ERROR, &code);
        aio4c_free(selector->polls);
        aio4c_free(selector);
        return NULL;
    }

#ifndef AIO4C_HAVE_PIPE
#ifndef AIO4C_WIN32
    if ((selector->pipe[AIO4C_PIPE_WRITE] = socket(AF_INET, SOCK_DGRAM, 0)) == -1) {
        code.error = errno;
#else /* AIO4C_WIN32 */
    if ((selector->pipe[AIO4C_PIPE_WRITE] = socket(AF_INET, SOCK_DGRAM, 0)) == INVALID_SOCKET) {
        code.source = AIO4C_ERRNO_SOURCE_WSA;
#endif /* AIO4C_WIN32 */
        code.selector = selector;
        Raise(AIO4C_LOG_LEVEL_ERROR, AIO4C_THREAD_SELECTOR_ERROR_TYPE, AIO4C_THREAD_SELECTOR_INIT_ERROR, &code);
#ifndef AIO4C_WIN32
        close(selector->pipe[AIO4C_PIPE_READ]);
#else /* AIO4C_WIN32 */
        closesocket(selector->pipe[AIO4C_PIPE_READ]);
#endif /* AIO4C_WIN32 */
        aio4c_free(selector->polls);
        aio4c_free(selector);
        return NULL;
    }
#endif /* AIO4C_HAVE_PIPE */

#ifndef AIO4C_WIN32
    if (fcntl(selector->pipe[AIO4C_PIPE_READ], F_SETFL, O_NONBLOCK) != 0) {
        code.error = errno;
        code.selector = selector;
        Raise(AIO4C_LOG_LEVEL_ERROR, AIO4C_THREAD_SELECTOR_ERROR_TYPE, AIO4C_THREAD_SELECTOR_INIT_ERROR, &code);
        close(selector->pipe[AIO4C_PIPE_READ]);
        close(selector->pipe[AIO4C_PIPE_WRITE]);
#else /* AIO4C_WIN32 */
    unsigned long ioctl = 1;
    if (ioctlsocket(selector->pipe[AIO4C_PIPE_READ], FIONBIO, &ioctl) != 0) {
        code.source = AIO4C_ERRNO_SOURCE_WSA;
        code.selector = selector;
        Raise(AIO4C_LOG_LEVEL_ERROR, AIO4C_THREAD_SELECTOR_ERROR_TYPE, AIO4C_THREAD_SELECTOR_INIT_ERROR, &code);
        closesocket(selector->pipe[AIO4C_PIPE_READ]);
        closesocket(selector->pipe[AIO4C_PIPE_WRITE]);
#endif /* AIO4C_WIN32 */
        aio4c_free(selector->polls);
        aio4c_free(selector);
        return NULL;
    }

#ifndef AIO4C_WIN32
    if (fcntl(selector->pipe[AIO4C_PIPE_WRITE], F_SETFL, O_NONBLOCK) != 0) {
        code.error = errno;
        code.selector = selector;
        Raise(AIO4C_LOG_LEVEL_ERROR, AIO4C_THREAD_SELECTOR_ERROR_TYPE, AIO4C_THREAD_SELECTOR_INIT_ERROR, &code);
        close(selector->pipe[AIO4C_PIPE_READ]);
        close(selector->pipe[AIO4C_PIPE_WRITE]);
#else /* AIO4C_WIN32 */
    if (ioctlsocket(selector->pipe[AIO4C_PIPE_WRITE], FIONBIO, &ioctl) != 0) {
        code.source = AIO4C_ERRNO_SOURCE_WSA;
        code.selector = selector;
        Raise(AIO4C_LOG_LEVEL_ERROR, AIO4C_THREAD_SELECTOR_ERROR_TYPE, AIO4C_THREAD_SELECTOR_INIT_ERROR, &code);
        closesocket(selector->pipe[AIO4C_PIPE_READ]);
        closesocket(selector->pipe[AIO4C_PIPE_WRITE]);
#endif /* AIO4C_WIN32 */
        aio4c_free(selector->polls);
        aio4c_free(selector);
        return NULL;
    }

#ifndef AIO4C_HAVE_PIPE
    struct sockaddr_in sa;
    memset(&sa, 0, sizeof(struct sockaddr_in));
    sa.sin_family = AF_INET;
    sa.sin_addr.s_addr = htonl(INADDR_LOOPBACK);
    while (true) {
        selector->port = _selectorNextPort++;
        sa.sin_port = htons(selector->port);
#ifndef AIO4C_WIN32
        if (bind(selector->pipe[AIO4C_PIPE_READ], (struct sockaddr*)&sa, sizeof(struct sockaddr_in)) == -1) {
            if (errno != EADDRINUSE) {
                code.error = errno;
#else /* AIO4C_WIN32 */
        if (bind(selector->pipe[AIO4C_PIPE_READ], (struct sockaddr*)&sa, sizeof(struct sockaddr_in)) == SOCKET_ERROR) {
            if (WSAGetLastError() != WSAEADDRINUSE) {
                code.source = AIO4C_ERRNO_SOURCE_WSA;
#endif /* AIO4C_WIN32 */
                code.selector = selector;
                Raise(AIO4C_LOG_LEVEL_ERROR, AIO4C_THREAD_SELECTOR_ERROR_TYPE, AIO4C_THREAD_SELECTOR_INIT_ERROR, &code);
#ifndef AIO4C_WIN32
                close(selector->pipe[AIO4C_PIPE_READ]);
                close(selector->pipe[AIO4C_PIPE_WRITE]);
#else /* AIO4C_WIN32 */
                closesocket(selector->pipe[AIO4C_PIPE_READ]);
                closesocket(selector->pipe[AIO4C_PIPE_WRITE]);
#endif /* AIO4C_WIN32 */
                aio4c_free(selector->polls);
                aio4c_free(selector);
                return NULL;
            } else {
                continue;
            }
        }

        break;
    }
#endif /* AIO4C_HAVE_PIPE */

    selector->keys = NULL;
    selector->numKeys = 0;
    selector->maxKeys = 0;

#ifdef AIO4C_HAVE_POLL
    memset(selector->polls, 0, sizeof(aio4c_poll_t));
#else /* AIO4C_HAVE_POLL */
    memset(selector->polls, 0, sizeof(Poll));
#endif /* AIO4C_HAVE_POLL */

    selector->polls[0].fd = selector->pipe[AIO4C_PIPE_READ];
    selector->polls[0].events = AIO4C_OP_READ;

    selector->numPolls = 1;
    selector->maxPolls = 1;

    selector->curKey = -1;
    selector->curKeyCount = -1;

    selector->lock = NewLock();

    return selector;
}

SelectionKey* _NewSelectionKey(void) {
    SelectionKey* key = NULL;
    ErrorCode code = AIO4C_ERROR_CODE_INITIALIZER;

    if ((key = aio4c_malloc(sizeof(SelectionKey))) == NULL) {
#ifndef AIO4C_WIN32
        code.error = errno;
#else /* AIO4C_WIN32 */
        code.source = AIO4C_ERRNO_SOURCE_SYS;
#endif /* AIO4C_WIN32 */
        code.size = sizeof(SelectionKey);
        code.type = "SelectionKey";
        Raise(AIO4C_LOG_LEVEL_ERROR, AIO4C_ALLOC_ERROR_TYPE, AIO4C_ALLOC_ERROR, &code);

        return NULL;
    }

    memset(key, 0, sizeof(SelectionKey));

    key->poll = -1;

    return key;
}

SelectionKey* Register(Selector* selector, SelectionOperation operation, aio4c_socket_t fd, void* attachment) {
    SelectionKey** keys = NULL;
#ifdef AIO4C_HAVE_POLL
    aio4c_poll_t* polls = NULL;
#else /* AIO4C_HAVE_POLL */
    Poll*         polls = NULL;
#endif /* AIO4C_HAVE_POLL */
    SelectionKey* key = NULL;
    ErrorCode code = AIO4C_ERROR_CODE_INITIALIZER;
    int keyPresent = -1;
    int i = 0;

    TakeLock(selector->lock);

    SelectorWakeUp(selector);

    for (i = 0; i < selector->numKeys; i++) {
        if (selector->keys[i]->fd == fd) {
            keyPresent = i;
            break;
        }
    }

    if (keyPresent == -1) {
        if (selector->numKeys == selector->maxKeys) {
            if ((keys = aio4c_realloc(selector->keys, (selector->maxKeys + 1) * sizeof(SelectionKey*))) == NULL) {
#ifndef AIO4C_WIN32
                code.error = errno;
#else /* AIO4C_WIN32 */
                code.source = AIO4C_ERRNO_SOURCE_SYS;
#endif /* AIO4C_WIN32 */
                code.size = (selector->maxKeys + 1) * sizeof(SelectionKey*);
                code.type = "SelectionKey*";
                Raise(AIO4C_LOG_LEVEL_ERROR, AIO4C_ALLOC_ERROR_TYPE, AIO4C_ALLOC_ERROR, &code);
                ReleaseLock(selector->lock);
                return NULL;
            }

            selector->keys = keys;

            selector->keys[selector->maxKeys] = _NewSelectionKey();

            selector->maxKeys++;
        }

        if (selector->numPolls == selector->maxPolls) {
#ifdef AIO4C_HAVE_POLL
            if ((polls = aio4c_realloc(selector->polls, (selector->maxPolls + 1) * sizeof(aio4c_poll_t))) == NULL) {
#else /* AIO4C_HAVE_POLL */
            if ((polls = aio4c_realloc(selector->polls, (selector->maxPolls + 1) * sizeof(Poll))) == NULL) {
#endif /* AIO4C_HAVE_POLL */

#ifndef AIO4C_WIN32
                code.error = errno;
#else /* AIO4C_WIN32 */
                code.source = AIO4C_ERRNO_SOURCE_SYS;
#endif /* AIO4C_WIN32 */

#ifdef AIO4C_HAVE_POLL
                code.size = (selector->maxPolls + 1) * sizeof(aio4c_poll_t);
                code.type = "aio4c_poll_t";
#else /* AIO4C_HAVE_POLL */
                code.size = (selector->maxPolls + 1) * sizeof(Poll);
                code.type = "Poll";
#endif /* AIO4C_HAVE_POLL */
                Raise(AIO4C_LOG_LEVEL_ERROR, AIO4C_ALLOC_ERROR_TYPE, AIO4C_ALLOC_ERROR, &code);
                ReleaseLock(selector->lock);
                return NULL;
            }

            selector->polls = polls;

#ifdef AIO4C_HAVE_POLL
            memset(&selector->polls[selector->maxPolls], 0, sizeof(aio4c_poll_t));
#else /* AIO4C_HAVE_POLL */
            memset(&selector->polls[selector->maxPolls], 0, sizeof(Poll));
#endif /* AIO4C_HAVE_POLL */

            selector->maxPolls++;
        }

        memset(selector->keys[selector->numKeys], 0, sizeof(SelectionKey));
        selector->keys[selector->numKeys]->poll = -1;

        selector->keys[selector->numKeys]->index = selector->numKeys;
        selector->keys[selector->numKeys]->operation = operation;
        selector->keys[selector->numKeys]->fd = fd;
        selector->keys[selector->numKeys]->attachment = attachment;
        keyPresent = selector->numKeys;

#ifdef AIO4C_HAVE_POLL
        memset(&selector->polls[selector->numPolls], 0, sizeof(aio4c_poll_t));
#else /* AIO4C_HAVE_POLL */
        memset(&selector->polls[selector->numPolls], 0, sizeof(Poll));
#endif /* AIO4C_HAVE_POLL */
        selector->polls[selector->numPolls].events = operation;
        selector->polls[selector->numPolls].fd = fd;
        selector->keys[selector->numKeys]->poll = selector->numPolls;
        selector->numPolls++;

        selector->numKeys++;
    }

    key = selector->keys[keyPresent];
    selector->keys[keyPresent]->count++;

    ReleaseLock(selector->lock);

    return key;
}

void Unregister(Selector* selector, SelectionKey* key, aio4c_bool_t unregisterAll) {
    int i = 0, j = 0;
    int pollIndex = key->poll;

    TakeLock(selector->lock);

    SelectorWakeUp(selector);

    if (key->index == -1) {
        ReleaseLock(selector->lock);
        return;
    }

    if (!unregisterAll) {
        key->count--;

        if (key->count > 0) {
            ReleaseLock(selector->lock);
            return;
        }
    }

    for (i = key->index; i < selector->numKeys - 1; i++) {
        memcpy(&selector->keys[i], &selector->keys[i + 1], sizeof(SelectionKey*));
        selector->keys[i]->index = i;
    }

    selector->keys[i] = key;
    memset(selector->keys[i], 0, sizeof(SelectionKey));
    selector->keys[i]->index = -1;

    selector->numKeys--;

    for (i = pollIndex; i < selector->numPolls - 1; i++) {
        for (j = 0; j < selector->numKeys; j++) {
            if (selector->keys[j]->poll == i + 1) {
                selector->keys[j]->poll--;
            }
        }

#ifdef AIO4C_HAVE_POLL
        memcpy(&selector->polls[i], &selector->polls[i + 1], sizeof(aio4c_poll_t));
#else /* AIO4C_HAVE_POLL */
        memcpy(&selector->polls[i], &selector->polls[i + 1], sizeof(Poll));
#endif /* AIO4C_HAVE_POLL */
    }

#ifdef AIO4C_HAVE_POLL
    memset(&selector->polls[i], 0, sizeof(aio4c_poll_t));
#else /* AIO4C_HAVE_POLL */
    memset(&selector->polls[i], 0, sizeof(Poll));
#endif /* AIO4C_HAVE_POLL */

    selector->numPolls--;

    ReleaseLock(selector->lock);
}

aio4c_size_t _Select(char* file, int line, Selector* selector) {
    int nbPolls = 0;
    unsigned char dummy = 0;
    ErrorCode code = AIO4C_ERROR_CODE_INITIALIZER;
    Thread* current = ThreadSelf();
    char* name = (current!=NULL)?current->name:NULL;

    TakeLock(selector->lock);

#ifndef AIO4C_HAVE_POLL
    fd_set wSet, rSet, eSet;
    int i = 0, maxFd = 0;
    aio4c_bool_t fdAdded = false;

    FD_ZERO(&wSet);
    FD_ZERO(&rSet);
    FD_ZERO(&eSet);

    FD_SET(selector->pipe[AIO4C_PIPE_READ], &rSet);
    maxFd = selector->pipe[AIO4C_PIPE_READ];

    for (i = 0; i < selector->numPolls; i++) {
        fdAdded = false;

        if (selector->polls[i].fd >= FD_SETSIZE) {
            Log(AIO4C_LOG_LEVEL_WARN, "descriptor %d overpasses FD_SETSIZE (%d), ignoring", selector->polls[i].fd, FD_SETSIZE);
            continue;
        }

        if (selector->polls[i].events & AIO4C_OP_READ) {
            FD_SET(selector->polls[i].fd, &rSet);
            fdAdded = true;
        }

        if (selector->polls[i].events & AIO4C_OP_WRITE) {
            FD_SET(selector->polls[i].fd, &wSet);
            fdAdded = true;
        }

        if (fdAdded) {
            FD_SET(selector->polls[i].fd, &eSet);
            if (selector->polls[i].fd > maxFd) {
                maxFd = selector->polls[i].fd;
            }
        }
    }

    maxFd++;
#endif /* AIO4C_HAVE_POLL */

    dthread("%s:%d: %s SELECT ON %p\n", file, line, name, (void*)selector);

#ifndef AIO4C_WIN32

#ifdef AIO4C_HAVE_POLL
    if ((nbPolls = poll(selector->polls, selector->numPolls, -1)) < 0) {
#else /* AIO4C_HAVE_POLL */
    if ((nbPolls = select(maxFd, &rSet, &wSet, &eSet, NULL)) < 0) {
#endif /* AIO4C_HAVE_POLL */
        if (errno != EINTR) {
            code.error = errno;
            code.selector = selector;
            Raise(AIO4C_LOG_LEVEL_WARN, AIO4C_THREAD_SELECTOR_ERROR_TYPE, AIO4C_THREAD_SELECTOR_SELECT_ERROR, &code);
            ReleaseLock(selector->lock);
            return 0;
        }
#else /* AIO4C_WIN32 */

#ifdef AIO4C_HAVE_POLL
    if ((nbPolls = WSAPoll(selector->polls, selector->numPolls, -1)) == SOCKET_ERROR) {
#else /* AIO4C_HAVE_POLL */
    if ((nbPolls = select(maxFd, &rSet, &wSet, &eSet, NULL)) == SOCKET_ERROR) {
#endif /* AIO4C_HAVE_POLL */
        code.source = AIO4C_ERRNO_SOURCE_WSA;
        code.selector = selector;
        Raise(AIO4C_LOG_LEVEL_WARN, AIO4C_THREAD_SELECTOR_ERROR_TYPE, AIO4C_THREAD_SELECTOR_SELECT_ERROR, &code);
        ReleaseLock(selector->lock);
        return 0;
#endif /* AIO4C_WIN32 */
    }

#ifdef AIO4C_HAVE_POLL
    if (selector->polls[0].revents & AIO4C_OP_READ) {
#else /* AIO4C_HAVE_POLL */
    if (FD_ISSET(selector->pipe[AIO4C_PIPE_READ], &rSet)) {
#endif /* AIO4C_HAVE_POLL */

        dthread("[SELECT SELECTOR %p] %s woken up\n", (void*)selector, name);

#ifndef AIO4C_WIN32
        if (read(selector->pipe[AIO4C_PIPE_READ], &dummy, sizeof(unsigned char)) < 0) {
            code.error = errno;
#else /* AIO4C_WIN32 */
        if (recv(selector->pipe[AIO4C_PIPE_READ], (void*)&dummy, sizeof(unsigned char), 0) == SOCKET_ERROR) {
            code.source= AIO4C_ERRNO_SOURCE_WSA;
#endif /* AIO4C_WIN32 */
            code.selector = selector;
            Raise(AIO4C_LOG_LEVEL_WARN, AIO4C_THREAD_SELECTOR_ERROR_TYPE, AIO4C_THREAD_SELECTOR_SELECT_ERROR, &code);
        }

#ifdef AIO4C_HAVE_POLL
        selector->polls[0].revents = 0;
#endif /* AIO4C_HAVE_POLL */

        nbPolls--;
    }

#ifndef AIO4C_HAVE_POLL
    for (i = 0; i < selector->numPolls; i++) {
        selector->polls[i].revents = 0;

        if (FD_ISSET(selector->polls[i].fd, &rSet)) {
            selector->polls[i].revents |= AIO4C_OP_READ;
        }
        if (FD_ISSET(selector->polls[i].fd, &wSet)) {
            selector->polls[i].revents |= AIO4C_OP_WRITE;
        }
        if (FD_ISSET(selector->polls[i].fd, &eSet)) {
            selector->polls[i].revents |= AIO4C_OP_ERROR;
        }
    }
#endif /* AIO4C_HAVE_POLL */

    dthread("[SELECT SELECTOR %p] %s select resulted in %d descriptor(s) ready\n", (void*)selector, name, nbPolls);

    ReleaseLock(selector->lock);

    return nbPolls;
}

void _SelectorWakeUp(char* file, int line, Selector* selector) {
    unsigned char dummy = 1;
    ErrorCode code = AIO4C_ERROR_CODE_INITIALIZER;
    Thread* current = ThreadSelf();
    char* name = (current!=NULL)?current->name:NULL;

    dthread("%s:%d: %s WAKING UP %p\n", file, line, name, (void*)selector);

#ifdef AIO4C_HAVE_POLL
    aio4c_poll_t polls[1] = { { .fd = selector->pipe[AIO4C_PIPE_WRITE], .events = AIO4C_OP_WRITE, .revents = 0 } };
#else /* AIO4C_HAVE_POLL */
    fd_set wSet, eSet;
    struct timeval tv = { .tv_sec = 0, .tv_usec = 0 };

    FD_ZERO(&wSet);
    FD_ZERO(&eSet);

    FD_SET(selector->pipe[AIO4C_PIPE_WRITE], &wSet);
    FD_SET(selector->pipe[AIO4C_PIPE_WRITE], &eSet);
#endif /* AIO4C_HAVE_POLL */

#ifndef AIO4C_WIN32
#ifdef AIO4C_HAVE_POLL
    if (poll(polls, 1, 0) < 0) {
#else /* AIO4C_HAVE_POLL */
    if (select(selector->pipe[AIO4C_PIPE_WRITE] + 1, NULL, &wSet, &eSet, &tv) < 0) {
#endif /* AIO4C_HAVE_POLL */
        code.error = errno;
#else /* AIO4C_WIN32 */
#ifdef AIO4C_HAVE_POLL
    if (WSAPoll(polls, 1, 0) == SOCKET_ERROR) {
#else /* AIO4C_HAVE_POLL */
    if (select(selector->pipe[AIO4C_PIPE_WRITE] + 1, NULL, &wSet, &eSet, &tv) < 0) {
#endif /* AIO4C_HAVE_POLL */
        code.source = AIO4C_ERRNO_SOURCE_WSA;
#endif /* AIO4C_WIN32 */
        code.selector = selector;
        Raise(AIO4C_LOG_LEVEL_WARN, AIO4C_THREAD_SELECTOR_ERROR_TYPE, AIO4C_THREAD_SELECTOR_WAKE_UP_ERROR, &code);
        return;
    }

#ifdef AIO4C_HAVE_POLL
    if (polls[0].revents & AIO4C_OP_WRITE) {
#else /* AIO4C_HAVE_POLL */
    if (FD_ISSET(selector->pipe[AIO4C_PIPE_WRITE], &wSet)) {
#endif /* AIO4C_HAVE_POLL */

#ifdef AIO4C_HAVE_PIPE
        if ((write(selector->pipe[AIO4C_PIPE_WRITE], &dummy, sizeof(unsigned char))) < 0) {
            code.error = errno;
#else /* AIO4C_HAVE_PIPE */
        struct sockaddr_in sa;
        memset(&sa, 0, sizeof(struct sockaddr_in));
        sa.sin_family = AF_INET;
        sa.sin_port = htons(selector->port);
        sa.sin_addr.s_addr = htonl(INADDR_LOOPBACK);
#ifndef AIO4C_WIN32
        if (sendto(selector->pipe[AIO4C_PIPE_WRITE], (void*)&dummy, sizeof(unsigned char), 0, (struct sockaddr*)&sa, sizeof(struct sockaddr_in)) == -1) {
            code.error = errno;
#else /* AIO4C_WIN32 */
        if (sendto(selector->pipe[AIO4C_PIPE_WRITE], (void*)&dummy, sizeof(unsigned char), 0, (struct sockaddr*)&sa, sizeof(struct sockaddr_in)) == SOCKET_ERROR) {
            code.source = AIO4C_ERRNO_SOURCE_WSA;
#endif /* AIO4C_WIN32 */
#endif /* AIO4C_HAVE_PIPE */
            code.selector = selector;
            Raise(AIO4C_LOG_LEVEL_WARN, AIO4C_THREAD_SELECTOR_ERROR_TYPE, AIO4C_THREAD_SELECTOR_WAKE_UP_ERROR, &code);
        }
    }
}

aio4c_bool_t SelectionKeyReady (Selector* selector, SelectionKey** key) {
    int i = 0;
    aio4c_bool_t result = true;
#ifdef AIO4C_HAVE_POLL
    aio4c_poll_t polls[1] = { { .fd = 0, .events = 0, .revents = 0 } };
#else /* AIO4C_HAVE_POLL */
    fd_set set, eSet;
    FD_ZERO(&set);
    FD_ZERO(&eSet);
    fd_set* wSet = NULL;
    fd_set* rSet = NULL;
    struct timeval tv = { .tv_sec = 0, .tv_usec = 0 };
#endif /* AIO4C_HAVE_POLL */
    ErrorCode code = AIO4C_ERROR_CODE_INITIALIZER;

    TakeLock(selector->lock);

    if (selector->curKey == -1) {
        selector->curKey = 0;
        selector->curKeyCount = 0;
    }

    if (selector->curKey >= selector->numKeys) {
        selector->curKey = -1;
        selector->curKeyCount = -1;
        result = false;
    }

    if (result) {
        if (selector->curKeyCount >= 1 && selector->keys[selector->curKey]->count > selector->curKeyCount) {
#ifdef AIO4C_HAVE_POLL
            polls[0].fd = selector->keys[selector->curKey]->fd;
            polls[0].events = selector->keys[selector->curKey]->operation;
#else /* AIO4C_HAVE_POLL */
            FD_SET(selector->keys[selector->curKey]->fd, &set);
            FD_SET(selector->keys[selector->curKey]->fd, &eSet);
            if (selector->keys[selector->curKey]->operation & AIO4C_OP_READ) {
                rSet = &set;
            } else {
                wSet = &set;
            }
#endif /* AIO4C_HAVE_POLL */

#ifndef AIO4C_WIN32
#ifdef AIO4C_HAVE_POLL
            if (poll(polls, 1, 0) < 0) {
#else /* AIO4C_HAVE_POLL */
            if (select(selector->keys[selector->curKey]->fd + 1, rSet, wSet, &eSet, &tv) < 0) {
#endif /* AIO4C_HAVE_POLL */
                code.error = errno;
#else /* AIO4C_WIN32 */
#ifdef AIO4C_HAVE_POLL
            if (WSAPoll(polls, 1, 0) == SOCKET_ERROR) {
#else /* AIO4C_HAVE_POLL */
            if (select(selector->keys[selector->curKey]->fd + 1, rSet, wSet, &eSet, &tv) == SOCKET_ERROR) {
#endif /* AIO4C_HAVE_POLL */
                code.source = AIO4C_ERRNO_SOURCE_WSA;
#endif /* AIO4C_WIN32 */
                code.selector = selector;
                Raise(AIO4C_LOG_LEVEL_ERROR, AIO4C_THREAD_SELECTOR_ERROR_TYPE, AIO4C_THREAD_SELECTOR_SELECT_ERROR, &code);
#ifdef AIO4C_HAVE_POLL
            } else if (polls[0].revents & polls[0].events) {
                selector->keys[selector->curKey]->result = polls[0].revents;
#else /* AIO4C_HAVE_POLL */
            } else if (FD_ISSET(selector->keys[selector->curKey]->fd, &set)) {
                if (FD_ISSET(selector->keys[selector->curKey]->fd, &eSet)) {
                    selector->keys[selector->curKey]->result |= AIO4C_OP_ERROR;
                }
                selector->keys[selector->curKey]->result |= selector->keys[selector->curKey]->operation;
#endif /* AIO4C_HAVE_POLL */
                selector->curKeyCount++;
                *key = selector->keys[selector->curKey];
                ReleaseLock(selector->lock);
                return true;
            } else {
                selector->curKeyCount = 0;
                selector->curKey++;
            }
        }

        for (i = selector->curKey; i < selector->numKeys; i++) {
#ifdef AIO4C_HAVE_POLL
            if (selector->polls[selector->keys[i]->poll].revents > 0) {
#else /* AIO4C_HAVE_POLL */
            if (selector->polls[selector->keys[i]->poll].revents > 0) {
#endif /* AIO4C_HAVE_POLL */
                selector->keys[i]->result = selector->polls[selector->keys[i]->poll].revents;
                *key = selector->keys[i];
                selector->curKeyCount++;
                selector->curKey = i + 1;
                selector->polls[selector->keys[i]->poll].revents = 0;
                break;
            }
        }

        if (i == selector->numKeys) {
            selector->curKey = -1;
            result = false;
        }
    }

    ReleaseLock(selector->lock);

    return result;
}

void FreeSelector(Selector** pSelector) {
    Selector* selector = NULL;
    int i = 0;

    if (pSelector != NULL && (selector = *pSelector) != NULL) {
#ifndef AIO4C_WIN32
        close(selector->pipe[AIO4C_PIPE_WRITE]);
        close(selector->pipe[AIO4C_PIPE_READ]);
#else /* AIO4C_WIN32 */
        closesocket(selector->pipe[AIO4C_PIPE_WRITE]);
        closesocket(selector->pipe[AIO4C_PIPE_READ]);
#endif /* AIO4C_WIN32 */
        if (selector->keys != NULL) {
            for (i = 0; i < selector->maxKeys; i++) {
                if (selector->keys[i] != NULL) {
                    aio4c_free(selector->keys[i]);
                }
            }
            aio4c_free(selector->keys);
        }
        selector->numKeys = 0;
        selector->maxKeys = 0;
        selector->curKey = 0;
        if (selector->polls != NULL) {
            aio4c_free(selector->polls);
        }
        selector->numPolls = 0;
        selector->maxPolls = 0;
        FreeLock(&selector->lock);
        aio4c_free(selector);
        *pSelector = NULL;
    }
}
