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
#include <aio4c/list.h>
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

    AIO4C_LIST_INITIALIZER(&selector->freeKeys);
    AIO4C_LIST_INITIALIZER(&selector->busyKeys);

#ifdef AIO4C_HAVE_POLL
    memset(selector->polls, 0, sizeof(aio4c_poll_t));
#else /* AIO4C_HAVE_POLL */
    memset(selector->polls, 0, sizeof(Poll));
#endif /* AIO4C_HAVE_POLL */

    selector->polls[0].fd = selector->pipe[AIO4C_PIPE_READ];
    selector->polls[0].events = AIO4C_OP_READ;

    selector->numPolls = 1;
    selector->maxPolls = 1;

    selector->curKey = NULL;

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
#ifdef AIO4C_HAVE_POLL
    aio4c_poll_t* polls = NULL;
#else /* AIO4C_HAVE_POLL */
    Poll*         polls = NULL;
#endif /* AIO4C_HAVE_POLL */
    SelectionKey* key = NULL;
    ErrorCode code = AIO4C_ERROR_CODE_INITIALIZER;
    Node* keyPresent = NULL;

    TakeLock(selector->lock);

    for (keyPresent = selector->busyKeys.first; keyPresent != NULL; keyPresent = keyPresent->next) {
        if (((SelectionKey*)keyPresent->data)->fd == fd) {
            break;
        }
    }

    if (keyPresent == NULL) {
        if (ListEmpty(&selector->freeKeys)) {
            keyPresent = NewNode(_NewSelectionKey());
        } else {
            keyPresent = ListPop(&selector->freeKeys);
        }

        ListAddLast(&selector->busyKeys, keyPresent);

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

        key = (SelectionKey*)keyPresent->data;
        key->poll = selector->numPolls;
        key->node = keyPresent;
        key->operation = operation;
        key->fd = fd;
        key->attachment = attachment;
        key->count = 0;
        key->curCount = 0;

#ifdef AIO4C_HAVE_POLL
        memset(&selector->polls[selector->numPolls], 0, sizeof(aio4c_poll_t));
#else /* AIO4C_HAVE_POLL */
        memset(&selector->polls[selector->numPolls], 0, sizeof(Poll));
#endif /* AIO4C_HAVE_POLL */
        selector->polls[selector->numPolls].events = operation;
        selector->polls[selector->numPolls].fd = fd;
        selector->numPolls++;
    }

    key = (SelectionKey*)keyPresent->data;
    key->count++;

    ReleaseLock(selector->lock);

    return key;
}

void Unregister(Selector* selector, SelectionKey* key, aio4c_bool_t unregisterAll, aio4c_bool_t* isLastRegistration) {
    int i = 0;
    int pollIndex = key->poll;
    SelectionKey* curKey = NULL;
    Node* node = key->node;

    TakeLock(selector->lock);

    if (isLastRegistration != NULL) {
        *isLastRegistration = true;
    }

    if (key->node == NULL) {
        ReleaseLock(selector->lock);
        return;
    }

    if (!unregisterAll) {
        key->count--;

        if (key->count > 0) {
            if (isLastRegistration != NULL) {
                *isLastRegistration = false;
            }
            ReleaseLock(selector->lock);
            return;
        }
    }

    ListRemove(&selector->busyKeys, node);
    memset(key, 0, sizeof(SelectionKey));
    ListAddLast(&selector->freeKeys, node);

    for (i = pollIndex; i < selector->numPolls - 1; i++) {
        for (node = selector->busyKeys.first; node != NULL; node = node->next) {
            curKey = node->data;
            if (curKey->poll == i + 1) {
                curKey->poll--;
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
    while ((nbPolls = poll(selector->polls, selector->numPolls, -1)) < 0) {
#else /* AIO4C_HAVE_POLL */
    while ((nbPolls = select(maxFd, &rSet, &wSet, &eSet, NULL)) < 0) {
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
    while ((nbPolls = WSAPoll(selector->polls, selector->numPolls, -1)) == SOCKET_ERROR) {
#else /* AIO4C_HAVE_POLL */
    while ((nbPolls = select(maxFd, &rSet, &wSet, &eSet, NULL)) == SOCKET_ERROR) {
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
    Node* i = NULL;
    SelectionKey* curKey = NULL;
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

    if (selector->curKey != NULL && selector->curKey->curCount >= 1) {
        if (selector->curKey->count > selector->curKey->curCount) {
#ifdef AIO4C_HAVE_POLL
            polls[0].fd = selector->curKey->fd;
            polls[0].events = selector->curKey->operation;
#else /* AIO4C_HAVE_POLL */
            FD_SET(selector->curKey->fd, &set);
            FD_SET(selector->curKey->fd, &eSet);
            if (selector->curKey->operation & AIO4C_OP_READ) {
                rSet = &set;
            } else {
                wSet = &set;
            }
#endif /* AIO4C_HAVE_POLL */

#ifndef AIO4C_WIN32
#ifdef AIO4C_HAVE_POLL
            if (poll(polls, 1, 0) < 0) {
#else /* AIO4C_HAVE_POLL */
            if (select(selector->curKey->fd + 1, rSet, wSet, &eSet, &tv) < 0) {
#endif /* AIO4C_HAVE_POLL */
                code.error = errno;
#else /* AIO4C_WIN32 */
#ifdef AIO4C_HAVE_POLL
            if (WSAPoll(polls, 1, 0) == SOCKET_ERROR) {
#else /* AIO4C_HAVE_POLL */
            if (select(selector->curKey->fd + 1, rSet, wSet, &eSet, &tv) == SOCKET_ERROR) {
#endif /* AIO4C_HAVE_POLL */
                code.source = AIO4C_ERRNO_SOURCE_WSA;
#endif /* AIO4C_WIN32 */
                code.selector = selector;
                Raise(AIO4C_LOG_LEVEL_ERROR, AIO4C_THREAD_SELECTOR_ERROR_TYPE, AIO4C_THREAD_SELECTOR_SELECT_ERROR, &code);
#ifdef AIO4C_HAVE_POLL
            } else if (polls[0].revents & polls[0].events) {
                selector->curKey->result = polls[0].revents;
#else /* AIO4C_HAVE_POLL */
            } else if (FD_ISSET(selector->curKey->fd, &set)) {
                if (FD_ISSET(selector->curKey->fd, &eSet)) {
                    selector->curKey->result |= AIO4C_OP_ERROR;
                }
                selector->curKey->result |= selector->curKey->operation;
#endif /* AIO4C_HAVE_POLL */
                selector->curKey->curCount++;
                *key = selector->curKey;
                ReleaseLock(selector->lock);
                return true;
            } else {
                if (selector->curKey->node->next != NULL) {
                    selector->curKey = (SelectionKey*)selector->curKey->node->next->data;
                } else {
                    selector->curKey = NULL;
                }
            }
        } else {
             selector->curKey->curCount = 0;
             if (selector->curKey->node->next != NULL) {
                 selector->curKey = (SelectionKey*)selector->curKey->node->next->data;
             } else {
                 selector->curKey = NULL;
             }
        }
    }

    if (selector->curKey == NULL) {
        if (!ListEmpty(&selector->busyKeys)) {
            selector->curKey = (SelectionKey*)selector->busyKeys.first->data;
        } else {
            *key = NULL;
            return false;
        }
    }

    if (selector->curKey != NULL) {
        for (i = selector->curKey->node; i != NULL; i = i->next) {
            curKey = (SelectionKey*)i->data;
#ifdef AIO4C_HAVE_POLL
            if (selector->polls[curKey->poll].revents > 0) {
#else /* AIO4C_HAVE_POLL */
            if (selector->polls[curKey->poll].revents > 0) {
#endif /* AIO4C_HAVE_POLL */
                curKey->result = selector->polls[curKey->poll].revents;
                *key = curKey;
                curKey->curCount++;
                selector->curKey = curKey;
                selector->polls[curKey->poll].revents = 0;
                break;
            }
        }
    }

    if (i == NULL) {
        selector->curKey = NULL;
        *key = NULL;
        result = false;
    }

    ReleaseLock(selector->lock);

    return result;
}

void FreeSelector(Selector** pSelector) {
    Selector* selector = NULL;
    Node* i = NULL;

    if (pSelector != NULL && (selector = *pSelector) != NULL) {
#ifndef AIO4C_WIN32
        close(selector->pipe[AIO4C_PIPE_WRITE]);
        close(selector->pipe[AIO4C_PIPE_READ]);
#else /* AIO4C_WIN32 */
        closesocket(selector->pipe[AIO4C_PIPE_WRITE]);
        closesocket(selector->pipe[AIO4C_PIPE_READ]);
#endif /* AIO4C_WIN32 */
        while (!ListEmpty(&selector->busyKeys)) {
            i = ListPop(&selector->busyKeys);
            aio4c_free(i->data);
            FreeNode(&i);
        }
        while (!ListEmpty(&selector->freeKeys)) {
            i = ListPop(&selector->freeKeys);
            aio4c_free(i->data);
            FreeNode(&i);
        }
        selector->curKey = NULL;
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
