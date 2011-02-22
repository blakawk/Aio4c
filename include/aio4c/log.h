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
#ifndef __AIO4C_LOG_H__
#define __AIO4C_LOG_H__

#include <aio4c/buffer.h>
#include <aio4c/types.h>
#include <aio4c/thread.h>

#define MIN_LOG_MESSAGE_SIZE \
    strlen("[00:00:00.000 00/00/00] [level] \n")

#define MAX_LOG_MESSAGE_SIZE \
    (MIN_LOG_MESSAGE_SIZE + 256)

typedef enum e_LogLevel {
    FATAL = 0,
    ERROR = 1,
    WARN = 2,
    INFO = 3,
    DEBUG = 4
} LogLevel;

typedef struct s_Logger {
    LogLevel          level;
    aio4c_file_t*     file;
    Queue*            queue;
    Thread*           thread;
    aio4c_bool_t      exiting;
} Logger;

extern void LogInit(Thread* parent, LogLevel level, char* filename);

extern void Log(Thread* from, LogLevel level, char* message, ...);

extern void LogBuffer(Thread* from, LogLevel level, Buffer* buffer);

extern void LogEnd(void);

#endif
