/**
 * This file is part of Aio4c <http://aio4c.so>.
 *
 * Aio4c <http://aio4c.so> is free software: you
 * can  redistribute  it  and/or modify it under
 * the  terms  of the GNU General Public License
 * as published by the Free Software Foundation,
 * version 3 of the License.
 *
 * This  program is distributed in the hope that
 * it  will be useful, but WITHOUT ANY WARRANTY;
 * without   even   the   implied   warranty  of
 * MERCHANTABILITY  or  FITNESS FOR A PARTICULAR
 * PURPOSE.
 *
 * See  the  GNU General Public License for more
 * details.  You  should have received a copy of
 * the  GNU  General  Public  License along with
 * Aio4c    <http://aio4c.so>.   If   not,   see
 * <http://www.gnu.org/licenses/>.
 */
#ifndef __AIO4C_WORKER_H__
#define __AIO4C_WORKER_H__

#include <aio4c/buffer.h>
#include <aio4c/connection.h>
#include <aio4c/thread.h>
#include <aio4c/types.h>
#include <aio4c/writer.h>

typedef struct s_Worker {
    char*        name;
    char*        pipe;
    int          bufferSize;
    Thread*      thread;
    Writer*      writer;
    Queue*       queue;
    BufferPool*  pool;
} Worker;

extern AIO4C_API Worker* NewWorker(char* pipeName, aio4c_size_t bufferSize);

extern AIO4C_API void WorkerManageConnection(Worker* worker, Connection* connection);

extern AIO4C_API void WorkerEnd(Worker* worker);

#endif
