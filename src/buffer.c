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
#include <aio4c/buffer.h>

#include <aio4c/alloc.h>
#include <aio4c/log.h>
#include <aio4c/stats.h>
#include <aio4c/types.h>
#include <aio4c/thread.h>

#include <stdlib.h>
#include <string.h>

static Buffer* _NewBuffer(int size) {
    Buffer* buffer = NULL;

    if ((buffer = aio4c_malloc(sizeof(Buffer))) == NULL) {
        return NULL;
    }

    if ((buffer->data = aio4c_malloc(size * sizeof(aio4c_byte_t))) == NULL) {
        aio4c_free(buffer);
        return NULL;
    }

    buffer->size = size;
    buffer->position = 0;
    buffer->limit = size;

    return buffer;
}

static void _FreeBuffer(Buffer** buffer) {
    Buffer* pBuffer = NULL;

    if (buffer != NULL && ((pBuffer = *buffer) != NULL)) {
        if (pBuffer->data != NULL) {
            aio4c_free(pBuffer->data);
            pBuffer->data = NULL;
        }

        aio4c_free(pBuffer);
        *buffer = NULL;
    }
}

BufferPool* NewBufferPool(int batch, int bufferSize) {
    BufferPool* pool = NULL;
    int i = 0;

    if ((pool = aio4c_malloc(sizeof(BufferPool))) == NULL) {
        return NULL;
    }

    if ((pool->lock = NewLock()) == NULL) {
        aio4c_free(pool);
        return NULL;
    }

    if ((pool->buffers = aio4c_malloc(batch * sizeof(Buffer*))) == NULL) {
        aio4c_free(pool);
        return NULL;
    }

    for (i = 0; i < batch; i++) {
        if ((pool->buffers[i] = _NewBuffer(bufferSize)) == NULL) {
            break;
        }
        pool->buffers[i]->poolIndex = i;
        pool->buffers[i]->pool = pool;
    }

    if (i < batch) {
        for (; i >= 0; i--) {
            _FreeBuffer(&pool->buffers[i]);
        }
        free(pool->buffers);
        free(pool);
        return NULL;
    }

    pool->size = batch;
    pool->used = 0;
    pool->batch = batch;
    pool->bufferSize = bufferSize;

    return pool;
}

Buffer* AllocateBuffer(BufferPool* pool) {
    Buffer** buffers = NULL;
    Buffer* buffer = NULL;
    int i = 0;

    ProbeTimeStart(AIO4C_TIME_PROBE_BUFFER_ALLOCATION);

    TakeLock(pool->lock);

    if (pool->used == pool->size) {
        if ((buffers = aio4c_realloc(pool->buffers, (pool->size + pool->batch) * sizeof(Buffer*))) == NULL) {
            return NULL;
        }

        pool->buffers = buffers;

        for (i = 0; i < pool->batch; i++) {
            if ((pool->buffers[pool->size + i] = _NewBuffer(pool->bufferSize)) == NULL) {
                break;
            }
            pool->buffers[pool->size + i]->pool = pool;
            pool->buffers[pool->size + i]->poolIndex = pool->size + i;
        }

        pool->size += i;
    }

    BufferReset(pool->buffers[pool->used]);
    buffer = pool->buffers[pool->used];
    buffer->poolIndex = pool->used;
    pool->used++;

    ProbeSize(AIO4C_PROBE_BUFFER_ALLOCATED_SIZE, buffer->size);

    ReleaseLock(pool->lock);

    ProbeTimeEnd(AIO4C_TIME_PROBE_BUFFER_ALLOCATION);

    return buffer;
}

void ReleaseBuffer(Buffer** pBuffer) {
    Buffer* buffer = NULL;
    BufferPool* pool = NULL;
    int i = 0;

    ProbeTimeStart(AIO4C_TIME_PROBE_BUFFER_ALLOCATION);

    if (pBuffer != NULL && (buffer = *pBuffer) != NULL) {
        if ((pool = buffer->pool) == NULL) {
            _FreeBuffer(pBuffer);
            return;
        }

        TakeLock(pool->lock);

        for (i = buffer->poolIndex; i < pool->used - 1; i++) {
            pool->buffers[i] = pool->buffers[i + 1];
            pool->buffers[i]->poolIndex = i;
        }

        pool->buffers[i] = buffer;
        pool->buffers[i]->poolIndex = i;
        pool->used--;

        ProbeSize(AIO4C_PROBE_BUFFER_ALLOCATED_SIZE, -buffer->size);

        ReleaseLock(pool->lock);

        *pBuffer = NULL;
    }

    ProbeTimeEnd(AIO4C_TIME_PROBE_BUFFER_ALLOCATION);
}

void FreeBufferPool(BufferPool** pPool) {
    BufferPool* pool = NULL;
    int i = 0;

    if (pPool != NULL && (pool = *pPool) != NULL) {
        if (pool->lock != NULL) {
            TakeLock(pool->lock);
        }

        for (i = 0; i < pool->size; i++) {
            if (pool->buffers[i] != NULL) {
                _FreeBuffer(&pool->buffers[i]);
            }
        }

        pool->used = 0;
        pool->size = 0;

        aio4c_free(pool->buffers);

        if (pool->lock != NULL) {
            ReleaseLock(pool->lock);
            FreeLock(&pool->lock);
        }

        aio4c_free(pool);

        *pPool = NULL;
    }
}

Buffer* BufferFlip(Buffer* buffer) {
    buffer->limit = buffer->position;
    buffer->position = 0;

    return buffer;
}

Buffer* BufferPosition(Buffer* buffer, int position) {
    if (position >= buffer->limit) {
        Log(AIO4C_LOG_LEVEL_ERROR, "invalid position %d for buffer %p [lim: %d]", position, (void*)buffer, buffer->limit);
        return NULL;
    } else {
        buffer->position = position;
    }

    return buffer;
}

Buffer* BufferLimit(Buffer* buffer, int limit) {
    if (limit > buffer->size) {
        Log(AIO4C_LOG_LEVEL_ERROR, "invalid limit %d for buffer %p [cap: %d]", limit, (void*)buffer, buffer->size);
        return NULL;
    } else {
        buffer->limit = limit;
    }

    return buffer;
}

Buffer* BufferReset(Buffer* buffer) {
    memset(buffer->data, 0, buffer->size);
    buffer->position = 0;
    buffer->limit = buffer->size;

    return buffer;
}

Buffer* BufferCopy(Buffer* dst, Buffer* src) {
    BufferReset(dst);
    memcpy(dst->data, &src->data[src->position], src->limit - src->position);
    BufferLimit(dst, src->limit - src->position);

    return dst;
}

aio4c_bool_t BufferHasRemaining(Buffer* buffer) {
    return (buffer->position < buffer->limit);
}

