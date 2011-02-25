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
#ifndef __AIO4C_BUFFER_H__
#define __AIO4C_BUFFER_H__

#include <aio4c/thread.h>
#include <aio4c/types.h>

typedef struct s_BufferPool BufferPool;

typedef struct s_Buffer {
    BufferPool*   pool;
    int           poolIndex;
    int           size;
    aio4c_byte_t* data;
    int           position;
    int           limit;
} Buffer;

struct s_BufferPool {
    Buffer** buffers;
    int      size;
    int      used;
    int      batch;
    int      bufferSize;
    Lock*    lock;
};

extern BufferPool* NewBufferPool(int batch, int bufferSize);

extern Buffer* AllocateBuffer(BufferPool* pool);

extern void ReleaseBuffer(Buffer** buffer);

extern void FreeBufferPool(BufferPool** pool);

extern Buffer* BufferFlip(Buffer* buffer);

extern Buffer* BufferPosition(Buffer* buffer, int position);

extern Buffer* BufferLimit(Buffer* buffer, int limit);

extern Buffer* BufferReset(Buffer* buffer);

extern Buffer* BufferCopy(Buffer* dst, Buffer* src);

extern aio4c_bool_t BufferHasRemaining(Buffer* buffer);

#endif
