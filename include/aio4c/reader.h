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
#ifndef __AIO4C_READER_H__
#define __AIO4C_READER_H__

#include <aio4c/connection.h>
#include <aio4c/thread.h>
#include <aio4c/types.h>
#include <aio4c/worker.h>

typedef struct s_Reader {
    Thread*        thread;
    Queue*         queue;
    Selector*      selector;
    Worker*        worker;
} Reader;

extern Reader* NewReader(Thread* parent, char* name, aio4c_size_t bufferSize);

extern void ReaderManageConnection(Reader* reader, Connection* connection);

extern void ReaderEnd(Reader* reader);

#endif
