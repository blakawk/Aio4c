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

#include <errno.h>
#include <pthread.h>
#include <stdlib.h>
#include <string.h>

void* aio4c_malloc(aio4c_size_t size) {
    void* ptr = NULL;
    aio4c_size_t* sPtr = NULL;

    ProbeTimeStart(TIME_PROBE_MEMORY_ALLOCATION);

    if ((ptr = malloc(size + sizeof(size))) == NULL) {
        Log(ThreadSelf(), FATAL, "cannot allocate memory: %s", strerror(errno));
        return NULL;
    }

    memset(ptr, 0, size + sizeof(size));

    ProbeSize(PROBE_MEMORY_ALLOCATED_SIZE, size);
    ProbeSize(PROBE_MEMORY_ALLOCATE_COUNT, 1);

    sPtr = (aio4c_size_t*)ptr;
    sPtr[0] = size;

    ProbeTimeEnd(TIME_PROBE_MEMORY_ALLOCATION);

    return (void*)(&sPtr[1]);
}

void* aio4c_realloc(void* ptr, aio4c_size_t size) {
    void* _ptr = NULL;
    aio4c_size_t* sPtr = NULL;
    aio4c_size_t prevSize = 0;
    unsigned char* cPtr = NULL;

    if (ptr == NULL) {
        return aio4c_malloc(size);
    }

    ProbeTimeStart(TIME_PROBE_MEMORY_ALLOCATION);

    sPtr = (aio4c_size_t*)ptr;

    prevSize = sPtr[-1];
    ptr = &sPtr[-1];

    if ((_ptr = realloc(ptr, size + sizeof(size))) == NULL) {
        Log(ThreadSelf(), FATAL, "cannot re-allocate memory: %s", strerror(errno));
        return NULL;
    }

    sPtr = (aio4c_size_t*)_ptr;
    sPtr[0] = size;
    cPtr = (unsigned char*)&sPtr[1];

    if (size > prevSize) {
        memset(&cPtr[prevSize], 0, size - prevSize);
    }

    ProbeSize(PROBE_MEMORY_ALLOCATE_COUNT, 1);
    ProbeSize(PROBE_MEMORY_ALLOCATED_SIZE, size - prevSize);

    ProbeTimeEnd(TIME_PROBE_MEMORY_ALLOCATION);

    return (void*)(cPtr);
}

void aio4c_free(void* ptr) {
    aio4c_size_t* sPtr = NULL;
    aio4c_size_t size = 0;

    ProbeTimeStart(TIME_PROBE_MEMORY_ALLOCATION);

    sPtr = (aio4c_size_t*)ptr;
    size = sPtr[-1];
    ptr = &sPtr[-1];

    free(ptr);

    ProbeSize(PROBE_MEMORY_ALLOCATED_SIZE, -size);
    ProbeSize(PROBE_MEMORY_FREE_COUNT, 1);

    ProbeTimeEnd(TIME_PROBE_MEMORY_ALLOCATION);
}
