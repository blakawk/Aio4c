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
#include <aio4c/alloc.h>
#include <aio4c/buffer.h>

#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define BUFFER_SIZE 4096
#define TEST_STRING "abcdefghijklmnopqrstuvwxyz"

int main(int argc, char* argv[]) {
    Buffer* a = NULL;
    aio4c_byte_t* data = NULL;
    aio4c_byte_t b = 0;
    char* s = NULL;
    int i = 0;
    int d = 12345;
    size_t len = 0;
    Aio4cInit(argc, argv, NULL, NULL);

    s = aio4c_malloc(strlen(TEST_STRING) + 1);
    assert(s != NULL);
    memcpy(s, TEST_STRING, strlen(TEST_STRING) + 1);

    a = NewBuffer(BUFFER_SIZE);
    assert(a != NULL);
    assert(BufferGetCapacity(a) == BUFFER_SIZE);
    assert(BufferGetLimit(a) == BUFFER_SIZE);
    assert(BufferGetPosition(a) == 0);
    assert(BufferHasRemaining(a) == true);
    assert(BufferRemaining(a) == BUFFER_SIZE);
    assert((data = BufferGetBytes(a)) != NULL);
    for(i = 0; i < BUFFER_SIZE; i++) {
        assert(data[i] == 0);
    }
    assert(BufferPutInt(a, &d) == true);
    assert(BufferGetPosition(a) == sizeof(int));
    assert(BufferFlip(a) == a);
    assert(BufferGetPosition(a) == 0);
    assert(BufferGetLimit(a) == sizeof(int));
    d = 0;
    assert(BufferGetInt(a, &d) == true);
    assert(d == 12345);
    d = 45678;
    assert(BufferGetInt(a, &d) == false);
    assert(d == 45678);
    assert(BufferPutInt(a, &d) == false);
    assert(BufferReset(a) == a);
    assert(BufferPutString(a, s) == true);
    assert(BufferGetPosition(a) == (int)(strlen(s) + 1));
    assert(BufferFlip(a) == a);
    assert(BufferGetPosition(a) == 0);
    assert(BufferGetLimit(a) == (int)(strlen(s) + 1));
    for(i = 0; i < (int)(strlen(s) + 1); i++) {
        assert(BufferGetByte(a, &b) == true);
        assert(b == s[i]);
        assert(BufferGetPosition(a) == (i + 1));
    }
    assert(BufferGetByte(a, &b) == false);
    assert(BufferHasRemaining(a) == false);
    assert(BufferReset(a) == a);
    assert(BufferGetCapacity(a) == BUFFER_SIZE);
    assert(BufferGetLimit(a) == BUFFER_SIZE);
    assert(BufferGetPosition(a) == 0);
    assert(BufferHasRemaining(a) == true);
    assert((data = BufferGetBytes(a)) != NULL);
    assert(BufferLimit(a, strlen(s) + 1) == a);
    assert(BufferGetLimit(a) == (int)(strlen(s) + 1));
    len = strlen(s);
    s[len] = '0';
    for(i = 0; i < BUFFER_SIZE; i++) {
        assert(data[i] == 0);
    }
    assert(BufferPutString(a, s) == false);
    for(i = 0; i < BUFFER_SIZE; i++) {
        assert(data[i] == 0);
    }
    s[len] = '\0';
    assert(BufferPutString(a, s) == true);
    assert(BufferFlip(a) == a);
    for(i = 0; i < (int)(strlen(s) + 1); i++) {
        assert(BufferGetByte(a, &b) == true);
        assert(b == s[i]);
        assert(BufferGetPosition(a) == (i + 1));
    }
    assert(BufferHasRemaining(a) == false);
    Aio4cEnd();
    return 0;
}
