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
#include <aio4c.h>
#include <aio4c/selector.h>
#include <aio4c/thread.h>
#include <aio4c/types.h>

#include <assert.h>
#include <fcntl.h>
#include <signal.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

static int COUNT = 1000;
static Lock* lock = NULL;
static Selector* selector[2] = {NULL,NULL};
static int fds[2][2] = {{-1,-1},{-1,-1}};
static unsigned char token[2][8192];

static aio4c_bool_t init(int* type) {
    sigset_t set;

    assert(sigfillset(&set) == 0);
    assert(sigprocmask(SIG_BLOCK, &set, NULL) == 0);

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

static void end(int* type) {
    switch(*type) {
        case 0:
            FreeSelector(&selector[0]);
            assert(selector[0] == NULL);
            break;
        case 1:
            FreeSelector(&selector[1]);
            assert(selector[1] == NULL);
            break;
        default:
            break;
    }
}

static void consumer(void) {
    unsigned char dummy[8192];
    int i = 0;
    SelectionKey* mykey = NULL, *readkey = NULL, *writekey = NULL;
    aio4c_bool_t isLastRegistration = false;

    assert((readkey = Register(selector[0], AIO4C_OP_READ, fds[0][0], &token[0])) != NULL);
    assert(readkey->fd == fds[0][0]);
    assert(readkey->attachment == &token[0]);
    assert(readkey->operation == AIO4C_OP_READ);

    for (i = 0; i < 1000; i++) {
        assert(Select(selector[0]) == 1);
        assert(SelectionKeyReady(selector[0], &mykey) == true);
        assert(selector[0]->curKey == mykey);
        assert(mykey == readkey);
        assert(mykey->attachment == &token[0]);
        assert(mykey->fd == fds[0][0]);
        assert(mykey->result == AIO4C_OP_READ);
        assert(mykey->curCount == 1);
        assert(read(fds[0][0], dummy, 8192) == 8192);
        assert(memcmp(dummy, token[0], 8192) == 0);
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
    assert(write(fds[1][1], token[1], 8192) == 8192);
    assert(SelectionKeyReady(selector[0], &mykey) == false);
    assert(mykey == NULL);
    Unregister(selector[0], writekey, false, &isLastRegistration);
    assert(isLastRegistration);
}

static void producer(void) {
    unsigned char dummy[8192];
    int i = 0;
    SelectionKey* mykey = NULL, *readkey = NULL, *writekey = NULL;
    aio4c_bool_t isLastRegistration;

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
            assert(write(fds[0][1], token[0], 8192) == 8192);
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
    assert((readkey = Register(selector[1], AIO4C_OP_READ, fds[1][0], &token[1])) != NULL);
    assert(readkey->fd == fds[1][0]);
    assert(readkey->attachment == &token[1]);
    assert(readkey->operation == AIO4C_OP_READ);
    assert(Select(selector[1]) == 1);
    assert(SelectionKeyReady(selector[1], &mykey) == true);
    assert(mykey == readkey);
    assert(mykey->attachment == &token[1]);
    assert(mykey->fd == fds[1][0]);
    assert(mykey->result == AIO4C_OP_READ);
    assert(mykey->curCount == 1);
    assert(read(fds[1][0], dummy, 8192) == 8192);
    assert(memcmp(token[1], dummy, 8192) == 0);
    assert(SelectionKeyReady(selector[1], &mykey) == false);
    assert(mykey == NULL);
    Unregister(selector[1], readkey, false, &isLastRegistration);
    assert(isLastRegistration);
}

static aio4c_bool_t run(int* type) {
    aio4c_bool_t cont = true;

    TakeLock(lock);
    COUNT--;
    if (COUNT < 0) {
        cont = false;
    }
    ReleaseLock(lock);

    switch (*type) {
        case 0:
            consumer();
            break;
        case 1:
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

int main(int argc, char* argv[]) {
    int consumerType = 0, producerType = 1;
    Thread* consumer = NULL, * producer = NULL;
    struct sigaction sa;
    int ko = 1;
    int urandom = -1;

    Aio4cInit(argc, argv);
    memset(&sa, 0, sizeof(struct sigaction));
    sa.sa_handler = handler;

    assert(pipe(fds[0]) == 0);
    assert(pipe(fds[1]) == 0);

    assert((urandom = open("/dev/urandom", O_RDONLY)) >= 0);
    assert(read(urandom, token[0], 8192) == 8192);
    assert(read(urandom, token[1], 8192) == 8192);
    assert(close(urandom) == 0);

    assert((lock = NewLock()) != NULL);

    NewThread("consumer", aio4c_thread_init(init), aio4c_thread_run(run), aio4c_thread_exit(end), aio4c_thread_arg(&consumerType), &consumer);
    NewThread("producer", aio4c_thread_init(init), aio4c_thread_run(run), aio4c_thread_exit(end), aio4c_thread_arg(&producerType), &producer);

    if(consumer != NULL && producer != NULL) {
        ko = 0;
        assert(sigaction(SIGPIPE, &sa, NULL) == 0);

        ThreadJoin(consumer);
        ThreadJoin(producer);
    }

    FreeLock(&lock);

    assert(close(fds[0][0]) == 0);
    assert(close(fds[0][1]) == 0);
    assert(close(fds[1][0]) == 0);
    assert(close(fds[1][1]) == 0);

    Aio4cEnd();

    if (!ko) {
        printf("OK\n");
    } else {
        printf("FAIL\n");
    }

    return ko;
}

