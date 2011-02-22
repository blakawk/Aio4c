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

#include <aio4c/types.h>

typedef struct s_Buffer {
    int           size;
    aio4c_byte_t* data;
    int           position;
    int           limit;
} Buffer;

extern Buffer* NewBuffer(int size);

extern void FreeBuffer(Buffer** buffer);

extern Buffer* BufferFlip(Buffer* buffer);

extern Buffer* BufferPosition(Buffer* buffer, int position);

extern Buffer* BufferLimit(Buffer* buffer, int limit);

extern Buffer* BufferReset(Buffer* buffer);

extern Buffer* BufferCopy(Buffer* dst, Buffer* src);

extern aio4c_bool_t BufferHasRemaining(Buffer* buffer);

#endif
