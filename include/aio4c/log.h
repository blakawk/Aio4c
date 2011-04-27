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
#ifndef __AIO4C_LOG_H__
#define __AIO4C_LOG_H__

#include <aio4c/buffer.h>
#include <aio4c/thread.h>
#include <aio4c/types.h>

#include <stdarg.h>

#define AIO4C_LOG_THREAD_NAME_SIZE 32

#define AIO4C_LOG_PREFIX_SIZE \
    (sizeof("[00:00:00.000 00/00/00] [level] : \n") + AIO4C_LOG_THREAD_NAME_SIZE)
#define AIO4C_LOG_MESSAGE_SIZE \
    (AIO4C_LOG_PREFIX_SIZE + 256)
#define AIO4C_LOG_BUFFER_SIZE \
    (AIO4C_LOG_PREFIX_SIZE + sizeof("0x00000000: 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 ................"))

typedef enum e_LogLevel {
    AIO4C_LOG_LEVEL_FATAL = 0,
    AIO4C_LOG_LEVEL_ERROR = 1,
    AIO4C_LOG_LEVEL_WARN = 2,
    AIO4C_LOG_LEVEL_INFO = 3,
    AIO4C_LOG_LEVEL_DEBUG = 4,
    AIO4C_LOG_LEVEL_MAX = 5
} LogLevel;

typedef struct s_Logger {
    LogLevel          level;
    aio4c_file_t*     file;
    Queue*            queue;
    Thread*           thread;
    aio4c_bool_t      exiting;
    Lock*             lock;
} Logger;

extern LogLevel AIO4C_LOG_LEVEL;

extern char* AIO4C_LOG_FILE;

extern AIO4C_API void LogInit(void);

extern AIO4C_API void Log(LogLevel level, char* message, ...) __attribute__((format(printf, 2, 3)));

extern AIO4C_API void LogBuffer(LogLevel level, Buffer* buffer);

extern AIO4C_API void LogEnd(void);

#endif
