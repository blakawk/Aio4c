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
#ifndef __AIO4C_READER_H__
#define __AIO4C_READER_H__

#include <aio4c/connection.h>
#include <aio4c/thread.h>
#include <aio4c/types.h>
#include <aio4c/worker.h>

typedef struct s_Reader {
    char*          name;
    char*          pipe;
    Thread*        thread;
    Queue*         queue;
    Selector*      selector;
    Worker*        worker;
    int            load;
    int            bufferSize;
} Reader;

extern AIO4C_API Reader* NewReader(char* pipeName, aio4c_size_t bufferSize);

extern AIO4C_API void ReaderManageConnection(Reader* reader, Connection* connection);

extern AIO4C_API void ReaderEnd(Reader* reader);

#endif
