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
#include <aio4c/alloc.h>

#include <aio4c/log.h>
#include <aio4c/stats.h>
#include <aio4c/thread.h>
#include <aio4c/types.h>

#ifndef AIO4C_WIN32

#include <errno.h>

#endif /* AIO4C_WIN32 */

#include <stdlib.h>
#include <string.h>

void* aio4c_malloc(volatile aio4c_size_t size) {
    void* ptr = NULL;
    aio4c_size_t* sPtr = NULL;

    ProbeTimeStart(AIO4C_TIME_PROBE_MEMORY_ALLOCATION);

    if ((ptr = malloc(size + sizeof(size))) == NULL) {
        return NULL;
    }

    memset(ptr, 0, size + sizeof(size));

    ProbeSize(AIO4C_PROBE_MEMORY_ALLOCATED_SIZE, size);
    ProbeSize(AIO4C_PROBE_MEMORY_ALLOCATE_COUNT, 1);

    sPtr = (aio4c_size_t*)ptr;
    sPtr[0] = size;

    ProbeTimeEnd(AIO4C_TIME_PROBE_MEMORY_ALLOCATION);

    return (void*)(&sPtr[1]);
}

void* aio4c_realloc(volatile void* ptr, volatile aio4c_size_t size) {
    void* _ptr = NULL;
    void* __ptr = NULL;
    aio4c_size_t* sPtr = NULL;
    aio4c_size_t prevSize = 0;
    unsigned char* cPtr = NULL;

    if (ptr == NULL) {
        return aio4c_malloc(size);
    }

    ProbeTimeStart(AIO4C_TIME_PROBE_MEMORY_ALLOCATION);

    sPtr = (aio4c_size_t*)ptr;

    prevSize = sPtr[-1];
    __ptr = &sPtr[-1];

    if ((_ptr = realloc(__ptr, size + sizeof(size))) == NULL) {
        return NULL;
    }

    sPtr = (aio4c_size_t*)_ptr;
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

void aio4c_free(volatile void* ptr) {
    aio4c_size_t* sPtr = NULL;
    aio4c_size_t size = 0;
    void* _ptr = NULL;

    ProbeTimeStart(AIO4C_TIME_PROBE_MEMORY_ALLOCATION);

    sPtr = (aio4c_size_t*)ptr;
    size = sPtr[-1];
    _ptr = &sPtr[-1];

    free(_ptr);

    ProbeSize(AIO4C_PROBE_MEMORY_ALLOCATED_SIZE, -size);
    ProbeSize(AIO4C_PROBE_MEMORY_FREE_COUNT, 1);

    ProbeTimeEnd(AIO4C_TIME_PROBE_MEMORY_ALLOCATION);
}
