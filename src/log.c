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
#include <aio4c/log.h>

#include <aio4c/alloc.h>
#include <aio4c/lock.h>
#include <aio4c/queue.h>
#include <aio4c/thread.h>
#include <aio4c/types.h>

#include <ctype.h>
#include <errno.h>
#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/time.h>
#include <time.h>

typedef struct s_Logger {
    LogLevel          level;
    aio4c_file_t*     file;
    Queue*            queue;
    Thread*           thread;
    bool      exiting;
    bool      custom;
    void            (*handler)(void*,LogLevel,char*);
    void*             arg;
} Logger;

static Logger _logger = {
    .level = AIO4C_LOG_LEVEL_FATAL,
    .file = NULL,
    .queue = NULL,
    .thread = NULL,
    .exiting = false,
    .custom = false,
    .handler = NULL,
    .arg = NULL
};

LogLevel AIO4C_LOG_LEVEL = AIO4C_LOG_LEVEL_FATAL;

char* AIO4C_LOG_FILE = NULL;

typedef struct s_LogMessage {
    LogLevel level;
    char* content;
} LogMessage;

static char* _levelStr[AIO4C_LOG_LEVEL_MAX] = {
    "fatal",
    "error",
    "warn",
    "info",
    "debug"
};

static void _LogPrintMessage(Logger* logger, LogMessage* message) {
    aio4c_file_t* log = NULL;

    if (message == NULL) {
        return;
    }

    if (logger != NULL && !logger->exiting && logger->file != NULL) {
        log = logger->file;
    } else {
        log = stderr;
    }

    fprintf(log, "[%s] %s", _levelStr[message->level], message->content);

    fflush(log);

    aio4c_free(message->content);
    aio4c_free(message);
}

static bool _LogInit(ThreadData _logger) {
    Logger* logger = (Logger*)_logger;
    Log(AIO4C_LOG_LEVEL_INFO, "logging is initialized, logger tid is 0x%08lx", ThreadGetId(logger->thread));
    return true;
}

static bool _LogRun(ThreadData _logger) {
    Logger* logger = (Logger*)_logger;
    QueueItem* item = NewQueueItem();

    while (Dequeue(logger->queue, item, true)) {
        switch (QueueItemGetType(item)) {
            case AIO4C_QUEUE_ITEM_EXIT:
                FreeQueueItem(&item);
                return false;
            case AIO4C_QUEUE_ITEM_DATA:
                _LogPrintMessage(logger, QueueDataItemGet(item));
                break;
            default:
                break;
        }
    }

    FreeQueueItem(&item);
    return true;
}

static void _LogExit(ThreadData _logger) {
    Logger* logger = (Logger*)_logger;
    Log(AIO4C_LOG_LEVEL_INFO, "logging finished");

    _LogRun(_logger);

    fclose(logger->file);
    logger->file = NULL;

    FreeQueue(&logger->queue);
}

static void _LogDefaultHandler(Logger* logger, LogLevel level, char* message) {
    LogMessage* lm = aio4c_malloc(sizeof(LogMessage));

    if (lm != NULL) {
        lm->content = message;
        lm->level = level;
        if (logger != NULL && logger->queue != NULL) {
            if (!EnqueueDataItem(logger->queue, lm)) {
                _LogPrintMessage(logger, lm);
            }
        } else {
            _LogPrintMessage(logger, lm);
        }
    }
}

void LogInit(void (*handler)(void*,LogLevel,char*), void* logger) {
    static unsigned char initialized = 0;
    Logger* threadData = &_logger;

    if (initialized || initialized++) {
        return;
    }

    if (handler != NULL) {
        _logger.custom = true;
        _logger.handler = handler;
        _logger.arg = logger;
        return;
    } else {
        _logger.handler = aio4c_log_handler(_LogDefaultHandler);
        _logger.arg = aio4c_log_arg(&_logger);
        _logger.level = AIO4C_LOG_LEVEL;
    }

    if (AIO4C_LOG_FILE != NULL) {
        if ((_logger.file = fopen(AIO4C_LOG_FILE, "a")) == NULL) {
            _logger.file = stderr;
            Log(AIO4C_LOG_LEVEL_WARN, "open %s: %s, therefore logging will be performed on console instead", AIO4C_LOG_FILE, strerror(errno));
        }
    } else {
        _logger.file = stderr;
    }

    if ((_logger.queue = NewQueue()) == NULL) {
        Log(AIO4C_LOG_LEVEL_WARN, "cannot create log messages queue: %s, therefore no thread will be used", strerror(errno));
    } else {
        _logger.thread = NewThread(
                "logger",
                _LogInit,
                _LogRun,
                _LogExit,
                (ThreadData)threadData);
        if (_logger.thread == NULL) {
            Log(AIO4C_LOG_LEVEL_WARN, "cannot create logging thread, logging may slow down performances");
        } else if (!ThreadStart(_logger.thread)) {
            Log(AIO4C_LOG_LEVEL_WARN, "cannot start logging thread, logging may slow down performances");
            FreeThread(&_logger.thread);
        }
    }
}

void LogEnd(void) {
    Logger* logger = &_logger;

    if (logger->queue != NULL) {
        logger->exiting = true;
        if (EnqueueExitItem(logger->queue)) {
            ThreadJoin(logger->thread);
        }
    }
}

static void _logsnprintf(char* buffer, int* pos, int len, char* message, ...) __attribute__((format(printf,4,5)));

void _logsnprintf(char* buffer, int* pos, int len, char* message, ...) {
    va_list va;
    int printed = 0;

    va_start(va, message);
    printed = vsnprintf(&buffer[*pos], len - *pos, message, va);
    va_end(va);

    if (printed < 0) {
        printed = 0;
    }

    *pos = *pos + printed;
}

static void _logprefix(char** pMessage, int* pos, bool allocate) {
    Thread* from = NULL;
    char* message = NULL;

    if (allocate) {
        if ((message = aio4c_malloc(AIO4C_LOG_MESSAGE_SIZE)) == NULL) {
            *pos = -1;
            return;
        } else {
            *pos = 0;
        }
    } else {
        message = *pMessage;
    }

    from = ThreadSelf();

    if (from != NULL && ThreadGetName(from) != NULL) {
        _logsnprintf(message, pos, AIO4C_LOG_MESSAGE_SIZE, "%s: ", ThreadGetName(from));
    }

    *pMessage = message;
}

void Log(LogLevel level, char* message, ...) {
    va_list va;
    Logger* logger = &_logger;
    char* _message = NULL;
    int pos = 0;
    int len = 0;

    if ((!logger->custom && level > logger->level) || logger->handler == NULL) {
        return;
    }

    _logprefix(&_message, &pos, true);

    va_start(va, message);
    len = vsnprintf(&_message[pos], AIO4C_LOG_MESSAGE_SIZE - pos, message, va);
    va_end(va);

    if (len < 0) {
        len = 0;
    }

    pos += len;

    _logsnprintf(_message, &pos, AIO4C_LOG_MESSAGE_SIZE, "\n");

    if (logger->handler != NULL) {
        logger->handler(logger->arg, level, _message);
    }
}

void LogBuffer(LogLevel level, Buffer* buffer) {
    char c = '.';
    int i = 0;
    int j = 0;
    int len = 0;
    int pos = 0;
    int bpos = BufferGetPosition(buffer);
    int blim = BufferGetLimit(buffer);
    aio4c_byte_t* bdata = BufferGetBytes(buffer);
    char* text = NULL;
    Logger* logger = &_logger;

    if ((!logger->custom && level > logger->level) || logger->handler == NULL) {
        return;
    }

    len = AIO4C_LOG_BUFFER_SIZE * (blim - bpos) / 16;

    if ((blim - bpos) % 16) {
        len += AIO4C_LOG_BUFFER_SIZE;
    }

    len += AIO4C_LOG_MESSAGE_SIZE;

    text = aio4c_malloc(len);
    if (text == NULL) {
        return;
    }

    _logprefix(&text, &pos, false);

    _logsnprintf(text, &pos, len,
            "dumping buffer %p [pos: %u, lim: %u, cap: %u]\n",
            (void*)buffer, bpos, blim, BufferGetCapacity(buffer));

    for (i = bpos; i < blim; i += 16) {
        _logsnprintf(text, &pos, len, "0x%08x: ", i);

        for (j = 0; j < 16; j ++) {
            if (i + j < blim) {
                _logsnprintf(text, &pos, len, "%02x ",
                        (bdata[i + j] & 0xff));
            } else {
                _logsnprintf(text, &pos, len, "%s", "   ");
            }
        }

        for (j = 0; j < 16 && (i + j < blim); j ++) {
            c = (char)(bdata[i + j] & 0xff);
            if (!isprint((int)c)) {
                c = '.';
            }

            _logsnprintf(text, &pos, len, "%c", c);
        }

        _logsnprintf(text, &pos, len, "%c", '\n');
    }

    logger->handler(logger->arg, level, text);
}

