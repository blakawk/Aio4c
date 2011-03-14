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
#include <aio4c/error.h>
#include <aio4c/log.h>
#include <aio4c/queue.h>
#include <aio4c/stats.h>
#include <aio4c/types.h>

#include <string.h>
#include <wchar.h>

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

BufferPool* NewBufferPool(aio4c_size_t bufferSize) {
    BufferPool* pool = NULL;
    Buffer* buffer = NULL;
    int i = 0;

    if ((pool = aio4c_malloc(sizeof(BufferPool))) == NULL) {
        return NULL;
    }

    pool->buffers = NewQueue();

    for (i = 0; i < AIO4C_BUFFER_POOL_BATCH_SIZE; i++) {
        buffer = _NewBuffer(bufferSize);

        if (buffer == NULL) {
            break;
        }

        buffer->pool = pool;

        EnqueueDataItem(pool->buffers, buffer);
    }

    pool->batch = AIO4C_BUFFER_POOL_BATCH_SIZE;
    pool->bufferSize = bufferSize;

    return pool;
}

Buffer* AllocateBuffer(BufferPool* pool) {
    Buffer* buffer = NULL;
    QueueItem item;
    int i = 0;

    ProbeTimeStart(AIO4C_TIME_PROBE_BUFFER_ALLOCATION);

    if (!Dequeue(pool->buffers, &item, false)) {
        for (i = 0; i < pool->batch; i++) {
            buffer = _NewBuffer(pool->bufferSize);

            if (buffer == NULL) {
                break;
            }

            buffer->pool = pool;

            EnqueueDataItem(pool->buffers, buffer);
        }

        if (!Dequeue(pool->buffers, &item, false)) {
            return NULL;
        }
    }

    buffer = (Buffer*)item.content.data;

    ProbeSize(AIO4C_PROBE_BUFFER_ALLOCATED_SIZE, buffer->size);

    ProbeTimeEnd(AIO4C_TIME_PROBE_BUFFER_ALLOCATION);

    return buffer;
}

void ReleaseBuffer(Buffer** pBuffer) {
    Buffer* buffer = NULL;
    BufferPool* pool = NULL;

    ProbeTimeStart(AIO4C_TIME_PROBE_BUFFER_ALLOCATION);

    if (pBuffer != NULL && (buffer = *pBuffer) != NULL) {
        if ((pool = buffer->pool) == NULL) {
            _FreeBuffer(pBuffer);
            return;
        }

        BufferReset(buffer);

        EnqueueDataItem(pool->buffers, buffer);

        ProbeSize(AIO4C_PROBE_BUFFER_ALLOCATED_SIZE, -buffer->size);

        *pBuffer = NULL;
    }

    ProbeTimeEnd(AIO4C_TIME_PROBE_BUFFER_ALLOCATION);
}

void FreeBufferPool(BufferPool** pPool) {
    BufferPool* pool = NULL;
    QueueItem item;

    if (pPool != NULL && (pool = *pPool) != NULL) {
        while (Dequeue(pool->buffers, &item, false)) {
            _FreeBuffer((Buffer**)&item.content.data);
        }

        FreeQueue(&pool->buffers);

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

aio4c_bool_t BufferGet(Buffer* buffer, void* out, int size) {
    ErrorCode code = AIO4C_ERROR_CODE_INITIALIZER;

    if (buffer->position + size > buffer->limit) {
        code.buffer = buffer;
        Raise(AIO4C_LOG_LEVEL_WARN, AIO4C_BUFFER_ERROR_TYPE, AIO4C_BUFFER_UNDERFLOW_ERROR, &code);
        return false;
    }

    memcpy(out, &buffer->data[buffer->position], size);

    buffer->position += size;

    return true;
}

aio4c_bool_t BufferGetString(Buffer* buffer, char** out) {
    ErrorCode code = AIO4C_ERROR_CODE_INITIALIZER;
    int len = strlen((char*)&buffer->data[buffer->position]) + 1;

    if (len > buffer->limit) {
        code.buffer = buffer;
        Raise(AIO4C_LOG_LEVEL_WARN, AIO4C_BUFFER_ERROR_TYPE, AIO4C_BUFFER_UNDERFLOW_ERROR, &code);
        *out = NULL;
        return false;
    }

    *out = (char*)&buffer->data[buffer->position];

    buffer->position += len;

    return true;
}

aio4c_bool_t BufferGetStringUTF(Buffer* buffer, wchar_t** out) {
    ErrorCode code = AIO4C_ERROR_CODE_INITIALIZER;
    int len = (wcslen((wchar_t*)&buffer->data[buffer->position]) + 1) * sizeof(wchar_t);

    if (len > buffer->limit) {
        code.buffer = buffer;
        Raise(AIO4C_LOG_LEVEL_WARN, AIO4C_BUFFER_ERROR_TYPE, AIO4C_BUFFER_UNDERFLOW_ERROR, &code);
        *out = NULL;
        return false;
    }

    *out = (wchar_t*)&buffer->data[buffer->position];

    buffer->position += len;

    return true;
}

aio4c_bool_t BufferPut(Buffer* buffer, void* in, int size) {
    ErrorCode code = AIO4C_ERROR_CODE_INITIALIZER;

    if (buffer->position + size > buffer->limit) {
        code.buffer = buffer;
        Raise(AIO4C_LOG_LEVEL_WARN, AIO4C_BUFFER_ERROR_TYPE, AIO4C_BUFFER_OVERFLOW_ERROR, &code);
        return false;
    }

    memcpy(&buffer->data[buffer->position], in, size);

    buffer->position += size;

    return true;
}

