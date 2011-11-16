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
#include <aio4c.h>
#include <aio4c/selector.h>
#include <aio4c/thread.h>
#include <aio4c/types.h>

#include <assert.h>
#include <fcntl.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#ifndef AIO4C_WIN32
#include <netinet/in.h>
#include <arpa/inet.h>
#include <sys/types.h>
#include <sys/socket.h>
#else
#include <winsock.h>
#endif

#ifndef AIO4C_WIN32
#define SOCKET_ERROR -1
#endif

#define BUFSZ 4096

static int COUNT = 100;
static Lock* lock = NULL;
static Selector* selector[2] = {NULL,NULL};
static int fds[2][3] = {{-1,-1,-1},{-1,-1,-1}};
static struct sockaddr_in to[2];
static unsigned char token[2][BUFSZ];

static bool init(ThreadData _type) {
    int* type = (int*)_type;
    switch (*type) {
        case 1:
            assert((selector[1] = NewSelector()) != NULL);
            break;
        case 0:
            assert((selector[0] = NewSelector()) != NULL);
            break;
        default:
            break;
    }

    return true;
}

static void end(ThreadData _type) {
    int *type = (int*)_type;
    switch(*type) {
        case 0:
            FreeSelector(&selector[0]);
            assert(selector[0] == NULL);
#ifndef AIO4C_WIN32
            assert(close(fds[0][2]) != SOCKET_ERROR);
#else
            assert(closesocket(fds[0][2]) != SOCKET_ERROR);
#endif
            break;
        case 1:
            FreeSelector(&selector[1]);
            assert(selector[1] == NULL);
#ifndef AIO4C_WIN32
            assert(close(fds[1][2]) != SOCKET_ERROR);
#else
            assert(closesocket(fds[1][2]) != SOCKET_ERROR);
#endif
            break;
        default:
            break;
    }
}

static void consumer(void) {
    unsigned char dummy[BUFSZ];
    int i = 0;
    SelectionKey* mykey = NULL, *readkey = NULL, *writekey = NULL;
    bool isLastRegistration = false;

    assert((readkey = Register(selector[0], AIO4C_OP_READ, fds[0][2], &token[0])) != NULL);
    assert(readkey->fd == fds[0][2]);
    assert(readkey->attachment == &token[0]);
    assert(readkey->operation == AIO4C_OP_READ);

    for (i = 0; i < 1000; i++) {
        assert(Select(selector[0]) == 1);
        assert(SelectionKeyReady(selector[0], &mykey) == true);
        assert(selector[0]->curKey == mykey);
        assert(mykey == readkey);
        assert(mykey->attachment == &token[0]);
        assert(mykey->fd == fds[0][2]);
        assert(mykey->result == AIO4C_OP_READ);
        assert(mykey->curCount == 1);
        assert(recv(fds[0][2], (char*)dummy, BUFSZ, MSG_WAITALL) == BUFSZ);
        assert(memcmp(dummy, token[0], BUFSZ) == 0);
        assert(SelectionKeyReady(selector[0], &mykey) == false);
        assert(readkey->curCount == 0);
        assert(selector[0]->curKey == NULL);
        assert(mykey == NULL);
    }

    assert(SelectionKeyReady(selector[0], &mykey) == false);
    assert(mykey == NULL);
    Unregister(selector[0], readkey, false, &isLastRegistration);
    assert(isLastRegistration);
    assert((writekey = Register(selector[0], AIO4C_OP_WRITE, fds[1][1], &token[1])) != NULL);
    assert(writekey->fd == fds[1][1]);
    assert(writekey->attachment == &token[1]);
    assert(writekey->operation == AIO4C_OP_WRITE);
    assert(Select(selector[0]) == 1);
    assert(SelectionKeyReady(selector[0], &mykey) == true);
    assert(mykey == writekey);
    assert(mykey->attachment == &token[1]);
    assert(mykey->fd == fds[1][1]);
    assert(mykey->result == AIO4C_OP_WRITE);
    assert(send(fds[1][1], (char*)token[1], BUFSZ, 0) == BUFSZ);
    assert(SelectionKeyReady(selector[0], &mykey) == false);
    assert(mykey == NULL);
    Unregister(selector[0], writekey, false, &isLastRegistration);
    assert(isLastRegistration);
}

static void producer(void) {
    unsigned char dummy[BUFSZ];
    int i = 0;
    SelectionKey* mykey = NULL, *readkey = NULL, *writekey = NULL;
    bool isLastRegistration;


    for (i = 0; i < 1000; i++) {
        assert((writekey = Register(selector[1], AIO4C_OP_WRITE, fds[0][1], &token[0])) != NULL);
        assert(writekey->fd == fds[0][1]);
        assert(writekey->attachment == &token[0]);
        assert(writekey->operation == AIO4C_OP_WRITE);
        assert(writekey->count == i + 1);
    }

    assert(writekey->count == 1000);
    assert(Select(selector[1]) == 1);

    for (i = 0; i < 1000; ) {
        if (SelectionKeyReady(selector[1], &mykey)) {
            assert(mykey == writekey);
            assert(mykey->attachment == &token[0]);
            assert(mykey->fd == fds[0][1]);
            assert(mykey->result == AIO4C_OP_WRITE);
            assert(selector[1]->curKey == mykey);
            assert(mykey->curCount == i + 1);
            assert(send(fds[0][1], (char*)token[0], BUFSZ, 0) == BUFSZ);
            i++;
        } else {
            assert(Select(selector[1]) == 1);
        }
    }

    assert(SelectionKeyReady(selector[1], &mykey) == false);
    assert(mykey == NULL);
    assert(selector[1]->curKey == NULL);
    for (i = 0; i < 1000; i++) {
        Unregister(selector[1], writekey, false, &isLastRegistration);
        assert(isLastRegistration == (i + 1 == 1000));
    }
    assert((readkey = Register(selector[1], AIO4C_OP_READ, fds[1][2], &token[1])) != NULL);
    assert(readkey->fd == fds[1][2]);
    assert(readkey->attachment == &token[1]);
    assert(readkey->operation == AIO4C_OP_READ);
    assert(Select(selector[1]) == 1);
    assert(SelectionKeyReady(selector[1], &mykey) == true);
    assert(mykey == readkey);
    assert(mykey->attachment == &token[1]);
    assert(mykey->fd == fds[1][2]);
    assert(mykey->result == AIO4C_OP_READ);
    assert(mykey->curCount == 1);
    assert(recv(fds[1][2], (char*)dummy, BUFSZ, 0) == BUFSZ);
    assert(memcmp(token[1], dummy, BUFSZ) == 0);
    assert(SelectionKeyReady(selector[1], &mykey) == false);
    assert(mykey == NULL);
    Unregister(selector[1], readkey, false, &isLastRegistration);
    assert(isLastRegistration);
}

static bool run(ThreadData _type) {
    int* type = (int*)_type;
    bool cont = true;
    static int connected[2] = {0,0};

    TakeLock(lock);
    COUNT--;
    if (COUNT < 0) {
        cont = false;
    }
    ReleaseLock(lock);

    switch (*type) {
        case 0:
            if (!connected[0]) {
                assert((fds[0][2] = accept(fds[0][0], NULL, NULL)) != SOCKET_ERROR);
                assert(connect(fds[1][1], (struct sockaddr*)&to[1], sizeof(struct sockaddr_in)) != SOCKET_ERROR);
                connected[0] = 1;
            }
            consumer();
            break;
        case 1:
            if (!connected[1]) {
                assert(connect(fds[0][1], (struct sockaddr*)&to[0], sizeof(struct sockaddr_in)) != SOCKET_ERROR);
                assert((fds[1][2] = accept(fds[1][0], NULL, NULL)) != SOCKET_ERROR);
                connected[1] = 1;
            }
            producer();
            break;
        default:
            break;
    }

    return (cont);
}

void handler(int signal __attribute__((unused))) {
    assert(false);
}

void channel(int fd[2], struct sockaddr_in* to) {
    static int _port = 55555;
    struct sockaddr_in sa;
#ifndef AIO4C_WIN32
    int opt = 1;
#else
    char opt = 1;
#endif

    memset(&sa, 0, sizeof(struct sockaddr_in));
    memset(to, 0, sizeof(struct sockaddr_in));
    sa.sin_family = to->sin_family = AF_INET;
    sa.sin_port = to->sin_port = htons(_port++);
    sa.sin_addr.s_addr = inet_addr("127.0.0.1");
    to->sin_addr.s_addr = inet_addr("127.0.0.1");
    assert((fd[0] = socket(AF_INET, SOCK_STREAM, 0)) != SOCKET_ERROR);
    assert((fd[1] = socket(AF_INET, SOCK_STREAM, 0)) != SOCKET_ERROR);
    assert(setsockopt(fd[0], SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt)) != SOCKET_ERROR);
    assert(bind(fd[0], (struct sockaddr*)&sa, sizeof(struct sockaddr_in)) != SOCKET_ERROR);
    assert(listen(fd[0], 1) != SOCKET_ERROR);
}

int main(int argc, char* argv[]) {
    int consumerType = 0, producerType = 1;
    int* _consumerType = &consumerType, *_producerType = &producerType;
    Thread* consumer = NULL, * producer = NULL;
    int ko = 1;
#ifndef AIO4C_WIN32
    int urandom = -1;
#endif

    Aio4cInit(argc, argv, NULL, NULL);

    channel(fds[0], &to[0]);
    channel(fds[1], &to[1]);

#ifndef AIO4C_WIN32
    assert((urandom = open("/dev/urandom", O_RDONLY)) >= 0);
    assert(read(urandom, token[0], BUFSZ) == BUFSZ);
    assert(read(urandom, token[1], BUFSZ) == BUFSZ);
    assert(close(urandom) == 0);
#else
    int i = 0;
    srand(getpid());
    for (i = 0; i < BUFSZ; i++) {
        token[0][i] = (unsigned char)(rand() & 0xff);
        token[1][i] = (unsigned char)(rand() & 0xff);
    }
#endif

    assert((lock = NewLock()) != NULL);

    consumer = NewThread("consumer", init, run, end, (ThreadData)_consumerType);
    producer = NewThread("producer", init, run, end, (ThreadData)_producerType);

    assert(ThreadStart(consumer));
    assert(ThreadStart(producer));

    if(consumer != NULL && producer != NULL) {
        ko = 0;

        ThreadJoin(consumer);
        ThreadJoin(producer);
    }

    FreeLock(&lock);

#ifndef AIO4C_WIN32
    assert(close(fds[0][0]) == 0);
    assert(close(fds[0][1]) == 0);
    assert(close(fds[1][0]) == 0);
    assert(close(fds[1][1]) == 0);
#else
    assert(closesocket(fds[0][0]) == 0);
    assert(closesocket(fds[0][1]) == 0);
    assert(closesocket(fds[1][0]) == 0);
    assert(closesocket(fds[1][1]) == 0);
#endif

    Aio4cEnd();

    return ko;
}

