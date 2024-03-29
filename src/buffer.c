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
#include <aio4c/buffer.h>

#include <aio4c/alloc.h>
#include <aio4c/error.h>
#include <aio4c/log.h>
#include <aio4c/queue.h>
#include <aio4c/stats.h>
#include <aio4c/types.h>

#include <string.h>

struct s_Buffer {
    BufferPool*   pool;
    int           size;
    aio4c_byte_t* data;
    int           position;
    int           limit;
};

struct s_BufferPool {
    Queue*   buffers;
    int      batch;
    int      bufferSize;
};

Buffer* NewBuffer(int size) {
    Buffer* buffer = NULL;

    if ((buffer = aio4c_malloc(sizeof(Buffer))) == NULL) {
        return NULL;
    }

    if ((buffer->data = aio4c_malloc((size + 2) * sizeof(aio4c_byte_t))) == NULL) {
        aio4c_free(buffer);
        return NULL;
    }

    buffer->size = size;
    buffer->position = 0;
    buffer->limit = size;

    return buffer;
}

void FreeBuffer(Buffer** buffer) {
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
        buffer = NewBuffer(bufferSize);

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

int GetBufferPoolBufferSize(BufferPool* pool) {
    return pool->bufferSize;
}

Buffer* AllocateBuffer(BufferPool* pool) {
    Buffer* buffer = NULL;
    QueueItem* item = NewQueueItem();
    int i = 0;

    ProbeTimeStart(AIO4C_TIME_PROBE_BUFFER_ALLOCATION);

    if (!Dequeue(pool->buffers, item, false)) {
        for (i = 0; i < pool->batch; i++) {
            buffer = NewBuffer(pool->bufferSize);

            if (buffer == NULL) {
                break;
            }

            buffer->pool = pool;

            EnqueueDataItem(pool->buffers, buffer);
        }

        if (!Dequeue(pool->buffers, item, false)) {
            return NULL;
        }
    }

    buffer = (Buffer*)QueueDataItemGet(item);

    ProbeSize(AIO4C_PROBE_BUFFER_ALLOCATED_SIZE, buffer->size);

    ProbeTimeEnd(AIO4C_TIME_PROBE_BUFFER_ALLOCATION);

    FreeQueueItem(&item);

    return buffer;
}

void ReleaseBuffer(Buffer** pBuffer) {
    Buffer* buffer = NULL;
    BufferPool* pool = NULL;

    ProbeTimeStart(AIO4C_TIME_PROBE_BUFFER_ALLOCATION);

    if (pBuffer != NULL && (buffer = *pBuffer) != NULL) {
        if ((pool = buffer->pool) == NULL) {
            FreeBuffer(pBuffer);
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
    QueueItem* item = NewQueueItem();
    Buffer* buffer = NULL;

    if (pPool != NULL && (pool = *pPool) != NULL) {
        while (Dequeue(pool->buffers, item, false)) {
            buffer = QueueDataItemGet(item);
            FreeBuffer(&buffer);
        }

        FreeQueue(&pool->buffers);

        aio4c_free(pool);

        *pPool = NULL;
    }

    FreeQueueItem(&item);
}

Buffer* BufferFlip(Buffer* buffer) {
    buffer->limit = buffer->position;
    buffer->position = 0;

    return buffer;
}

Buffer* BufferPosition(Buffer* buffer, int position) {
    if (position > buffer->limit) {
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

bool BufferHasRemaining(Buffer* buffer) {
    return (buffer->position < buffer->limit);
}

bool BufferGet(Buffer* buffer, void* out, int size) {
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

bool BufferGetString(Buffer* buffer, char** out) {
    int len = strlen((char*)&buffer->data[buffer->position]) + 1;

    if (len > (buffer->limit - buffer->position)) {
        len = buffer->limit - buffer->position;
        buffer->data[buffer->limit] = '\0';
        buffer->data[buffer->limit + 1] = '\0';
    }

    *out = (char*)&buffer->data[buffer->position];
    buffer->position += len;

    return true;
}

bool BufferPut(Buffer* buffer, void* in, int size) {
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

bool BufferPutString(Buffer* buffer, char* in) {
    ErrorCode code = AIO4C_ERROR_CODE_INITIALIZER;
    size_t inLen = 0;

    if(memchr(in, '\0', buffer->limit - buffer->position) == NULL) {
        code.buffer = buffer;
        Raise(AIO4C_LOG_LEVEL_WARN, AIO4C_BUFFER_ERROR_TYPE, AIO4C_BUFFER_OVERFLOW_ERROR, &code);
        return false;
    }

    inLen = strlen(in) + 1;

    memcpy(&buffer->data[buffer->position], in, inLen);

    buffer->position += inLen;

    return true;
}

int BufferGetPosition(Buffer* buffer) {
    return buffer->position;
}

int BufferGetLimit(Buffer* buffer) {
    return buffer->limit;
}

int BufferGetCapacity(Buffer* buffer) {
    return buffer->size;
}

int BufferRemaining(Buffer* buffer) {
    return buffer->limit - buffer->position;
}

aio4c_byte_t* BufferGetBytes(Buffer* buffer) {
    return buffer->data;
}
