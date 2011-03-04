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
#ifndef __AIO4C_WORKER_H__
#define __AIO4C_WORKER_H__

#include <aio4c/buffer.h>
#include <aio4c/connection.h>
#include <aio4c/thread.h>
#include <aio4c/types.h>
#include <aio4c/writer.h>

typedef struct s_Task {
    Connection* connection;
    Buffer* buffer;
} Task;

typedef struct s_Worker {
    Thread*      thread;
    aio4c_size_t bufferSize;
    Writer*      writer;
    Queue*       queue;
    BufferPool*  pool;
} Worker;

extern Worker* NewWorker(aio4c_size_t bufferSize);

extern void WorkerManageConnection(Worker* worker, Connection* connection);

extern void WorkerEnd(Worker* worker);

#endif
