/**
 * This file is part of Aio4c <http://aio4c.so>.
 *
 * Aio4c <http://aio4c.so> is free software: you
 * can  redistribute  it  and/or modify it under
 * the  terms  of the GNU General Public License
 * as published by the Free Software Foundation,
 * version 3 of the License.
 *
 * This  program is distributed in the hope that
 * it  will be useful, but WITHOUT ANY WARRANTY;
 * without   even   the   implied   warranty  of
 * MERCHANTABILITY  or  FITNESS FOR A PARTICULAR
 * PURPOSE.
 *
 * See  the  GNU General Public License for more
 * details.  You  should have received a copy of
 * the  GNU  General  Public  License along with
 * Aio4c    <http://aio4c.so>.   If   not,   see
 * <http://www.gnu.org/licenses/>.
 */
#include <aio4c/alloc.h>

#include <aio4c/log.h>
#include <aio4c/stats.h>
#include <aio4c/thread.h>

#ifndef AIO4C_WIN32
#include <errno.h>
#endif /* AIO4C_WIN32 */

#include <stdlib.h>
#include <string.h>

void* aio4c_malloc(int size) {
    void* ptr = NULL;
    int* sPtr = NULL;

    ProbeTimeStart(AIO4C_TIME_PROBE_MEMORY_ALLOCATION);

    if ((ptr = malloc(size + sizeof(int))) == NULL) {
        return NULL;
    }

    memset(ptr, 0, size + sizeof(int));

    ProbeSize(AIO4C_PROBE_MEMORY_ALLOCATED_SIZE, size);
    ProbeSize(AIO4C_PROBE_MEMORY_ALLOCATE_COUNT, 1);

    sPtr = (int*)ptr;
    sPtr[0] = size;

    ProbeTimeEnd(AIO4C_TIME_PROBE_MEMORY_ALLOCATION);

    return (void*)(&sPtr[1]);
}

void* aio4c_realloc(void* ptr, int size) {
    void* _ptr = NULL;
    void* __ptr = NULL;
    int* sPtr = NULL;
    int prevSize = 0;
    unsigned char* cPtr = NULL;

    if (ptr == NULL) {
        return aio4c_malloc(size);
    }

    ProbeTimeStart(AIO4C_TIME_PROBE_MEMORY_ALLOCATION);

    sPtr = (int*)ptr;

    prevSize = sPtr[-1];

    if (prevSize == -1) {
        abort();
    }

    __ptr = &sPtr[-1];

    if ((_ptr = realloc(__ptr, size + sizeof(int))) == NULL) {
        return NULL;
    }

    sPtr = (int*)_ptr;
    sPtr[0] = size;
    cPtr = (unsigned char*)&sPtr[1];

    if (size > prevSize) {
        memset(&cPtr[prevSize], 0, size - prevSize);
    }

    ProbeSize(AIO4C_PROBE_MEMORY_ALLOCATE_COUNT, 1);
    ProbeSize(AIO4C_PROBE_MEMORY_ALLOCATED_SIZE, size - prevSize);

    ProbeTimeEnd(AIO4C_TIME_PROBE_MEMORY_ALLOCATION);

    return (void*)(cPtr);
}

void aio4c_free(void* ptr) {
    int* sPtr = NULL;
    int size = 0;
    void* _ptr = NULL;

    ProbeTimeStart(AIO4C_TIME_PROBE_MEMORY_ALLOCATION);

    sPtr = (int*)ptr;
    size = sPtr[-1];

    if (size == -1) {
        abort();
    }

    _ptr = &sPtr[-1];
    sPtr[-1] = -1;

    free(_ptr);

    ProbeSize(AIO4C_PROBE_MEMORY_ALLOCATED_SIZE, -size);
    ProbeSize(AIO4C_PROBE_MEMORY_FREE_COUNT, 1);

    ProbeTimeEnd(AIO4C_TIME_PROBE_MEMORY_ALLOCATION);
}
