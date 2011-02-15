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
#include <aio4c/types.h>
#include <stdlib.h>

Buffer* NewBuffer(aio4c_size_t size) {
    Buffer* buffer = NULL;

    if ((buffer = malloc(sizeof(Buffer))) == NULL) {
        return NULL;
    }

    if ((buffer->data = malloc(size * sizeof(aio4c_byte_t))) == NULL) {
        free(buffer);
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
            free(pBuffer->data);
            pBuffer->data = NULL;
        }

        free(pBuffer);
        *buffer = NULL;
    }
}

Buffer* BufferFlip(Buffer* buffer) {
    buffer->limit = buffer->position;
    buffer->position = 0;

    return buffer;
}

Buffer* BufferPosition(Buffer* buffer, aio4c_position_t position) {
    if (position >= buffer->limit) {
        FreeBuffer(&buffer);
    } else {
        buffer->position = position;
    }

    return buffer;
}

Buffer* BufferLimit(Buffer* buffer, aio4c_position_t limit) {
    if (limit >= buffer->size) {
        FreeBuffer(&buffer);
    } else {
        buffer->limit = limit;
    }

    return buffer;
}
