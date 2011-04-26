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

#include <aio4c/queue.h>
#include <aio4c/types.h>

#include <string.h>
#include <wchar.h>

#define AIO4C_BUFFER_POOL_BATCH_SIZE 16

typedef struct s_BufferPool BufferPool;

#ifndef __AIO4C_BUFFER_DEFINED__
#define __AIO4C_BUFFER_DEFINED__
typedef struct s_Buffer Buffer;
#endif /* __AIO4C_BUFFER_DEFINED__ */

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

extern AIO4C_API Buffer* NewBuffer(int size);

extern AIO4C_API void FreeBuffer(Buffer** buffer);

extern AIO4C_API BufferPool* NewBufferPool(aio4c_size_t bufferSize);

extern AIO4C_API Buffer* AllocateBuffer(BufferPool* pool);

extern AIO4C_API void ReleaseBuffer(Buffer** buffer);

extern AIO4C_API void FreeBufferPool(BufferPool** pool);

extern AIO4C_API Buffer* BufferFlip(Buffer* buffer);

extern AIO4C_API Buffer* BufferPosition(Buffer* buffer, int position);

extern AIO4C_API Buffer* BufferLimit(Buffer* buffer, int limit);

extern AIO4C_API Buffer* BufferReset(Buffer* buffer);

extern AIO4C_API Buffer* BufferCopy(Buffer* dst, Buffer* src);

extern AIO4C_API int BufferGetPosition(Buffer* buffer);

extern AIO4C_API int BufferGetLimit(Buffer* buffer);

extern AIO4C_API int BufferRemaining(Buffer* buffer);

extern AIO4C_API aio4c_bool_t BufferHasRemaining(Buffer* buffer);

extern AIO4C_API aio4c_byte_t* BufferGetBytes(Buffer* buffer);

#define BufferGetByte(buffer,out) \
    BufferGet(buffer, out, sizeof(aio4c_byte_t))

#define BufferGetShort(buffer,out) \
    BufferGet(buffer, out, sizeof(short))

#define BufferGetInt(buffer,out) \
    BufferGet(buffer, out, sizeof(int))

#define BufferGetLong(buffer,out) \
    BufferGet(buffer, out, sizeof(long))

#define BufferGetFloat(buffer,out) \
    BufferGet(buffer, out, sizeof(float))

#define BufferGetDouble(buffer,out) \
    BufferGet(buffer, out, sizeof(double))

extern AIO4C_API aio4c_bool_t BufferGet(Buffer* buffer, void* out, int size);

extern AIO4C_API aio4c_bool_t BufferGetString(Buffer* buffer, char** out);

extern AIO4C_API aio4c_bool_t BufferGetStringUTF(Buffer* buffer, wchar_t** out);

#define BufferPutByte(buffer, in) \
    BufferPut(buffer, in, sizeof(aio4c_byte_t))

#define BufferPutShort(buffer,in) \
    BufferPut(buffer, in, sizeof(short))

#define BufferPutInt(buffer,in) \
    BufferPut(buffer, in, sizeof(int))

#define BufferPutLong(buffer,in) \
    BufferPut(buffer, in, sizeof(long))

#define BufferPutFloat(buffer,in) \
    BufferPut(buffer, in, sizeof(float))

#define BufferPutDouble(buffer,in) \
    BufferPut(buffer, in, sizeof(double))

#define BufferPutString(buffer,in) \
    BufferPut(buffer, in, (strlen(in) + 1) * sizeof(char))

#define BufferPutStringUTF(buffer,in) \
    BufferPut(buffer, in, (wcslen(in) + 1) * sizeof(wchar_t))

extern AIO4C_API aio4c_bool_t BufferPut(Buffer* buffer, void* in, int size);

#endif
