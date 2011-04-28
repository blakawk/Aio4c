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

static Logger _logger = {
    .level = AIO4C_LOG_LEVEL_FATAL,
    .file = NULL,
    .queue = NULL,
    .thread = NULL,
    .exiting = false
};

LogLevel AIO4C_LOG_LEVEL = AIO4C_LOG_LEVEL_FATAL;
char* AIO4C_LOG_FILE = NULL;

static void _LogPrintMessage(Logger* logger, char* message) {
    aio4c_file_t* log = NULL;

    if (message == NULL) {
        return;
    }

    if (logger != NULL && !logger->exiting && logger->file != NULL) {
        log = logger->file;
    } else {
        log = stderr;
    }

    fprintf(log, "%s", message);
    fflush(log);
    aio4c_free(message);
}

static aio4c_bool_t _LogInit(Logger* logger) {
    Log(AIO4C_LOG_LEVEL_INFO, "logging is initialized, logger tid is 0x%08lx", logger->thread->id);
    return true;
}

static aio4c_bool_t _LogRun(Logger* logger) {
    QueueItem item;

    memset(&item, 0, sizeof(QueueItem));

    while (Dequeue(logger->queue, &item, true)) {
        switch (item.type) {
            case AIO4C_QUEUE_ITEM_EXIT:
                return false;
            case AIO4C_QUEUE_ITEM_DATA:
                _LogPrintMessage(logger, (char*)item.content.data);
                break;
            default:
                break;
        }
    }

    return true;
}

static void _LogExit(Logger* logger) {
    Log(AIO4C_LOG_LEVEL_INFO, "logging finished");

    _LogRun(logger);

    fclose(logger->file);
    logger->file = NULL;

    FreeQueue(&logger->queue);
}

void LogInit(void) {
    static unsigned char initialized = 0;

    if (initialized || initialized++) {
        return;
    }

    _logger.lock = NewLock();
    _logger.level = AIO4C_LOG_LEVEL;
    _logger.thread = NULL;
    _logger.exiting = false;

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
        _logger.thread = NULL;
        NewThread("logger",
               aio4c_thread_init(_LogInit),
               aio4c_thread_run(_LogRun),
               aio4c_thread_exit(_LogExit),
               aio4c_thread_arg(&_logger),
               &_logger.thread);
        if (_logger.thread == NULL) {
            Log(AIO4C_LOG_LEVEL_WARN, "cannot create logging thread, logging may slow down performances");
        }
    }
}

static aio4c_size_t _LogPrefix(Thread* from, LogLevel level, char** message) {
    char* pMessage = NULL;
    char* levelString = "";
    struct timeval tv;
    struct tm* tm = NULL;
    int prefixLen = 0, fromLen = 0;

    if (*message == NULL) {
        if ((pMessage = aio4c_malloc(AIO4C_LOG_MESSAGE_SIZE)) == NULL) {
            return 0;
        }
    } else {
        pMessage = *message;
    }

    memset(&tv, 0, sizeof(struct timeval));

    if (_logger.lock != NULL) {
        TakeLock(_logger.lock);
    }

    gettimeofday(&tv, NULL);
    tm = localtime((time_t*)&tv.tv_sec);

    switch(level) {
        case AIO4C_LOG_LEVEL_INFO:
            levelString = "info";
            break;
        case AIO4C_LOG_LEVEL_WARN:
            levelString = "warn";
            break;
        case AIO4C_LOG_LEVEL_ERROR:
            levelString = "error";
            break;
        case AIO4C_LOG_LEVEL_FATAL:
            levelString = "FATAL";
            break;
        case AIO4C_LOG_LEVEL_DEBUG:
            levelString = "debug";
            break;
        default:
            break;
    }

    prefixLen = snprintf(pMessage, AIO4C_LOG_PREFIX_SIZE, "[%02d:%02d:%02d.%03d %02d/%02d/%02d] [%s] ",
            tm->tm_hour, tm->tm_min, tm->tm_sec, (int)(tv.tv_usec / 1000), tm->tm_mday,
            tm->tm_mon + 1, tm->tm_year % 100, levelString);

    if (_logger.lock != NULL) {
        ReleaseLock(_logger.lock);
    }

    if (from == NULL) {
        from = ThreadSelf();
    }

    if (from != NULL && from->name != NULL) {
        fromLen = snprintf(&pMessage[prefixLen], AIO4C_LOG_MESSAGE_SIZE - prefixLen, "%s: ", from->name);
        if (fromLen < 0) {
            fromLen = 0;
        }
    }

    *message = pMessage;

    return prefixLen + fromLen;
}

void Log(LogLevel level, char* message, ...) {
    va_list args;
    Thread* from = ThreadSelf();
    Logger* logger = &_logger;
    char* pMessage = NULL;
    int messageLen = 0, prefixLen = 0;

    if (logger != NULL && level > logger->level) {
        return;
    }

    prefixLen = _LogPrefix(from, level, &pMessage);
    if (pMessage == NULL) {
        return;
    }

    va_start(args, message);
    messageLen = vsnprintf(&pMessage[prefixLen], AIO4C_LOG_MESSAGE_SIZE - prefixLen, message, args);
    va_end(args);

    if (messageLen < 0) {
        messageLen = 0;
    }

    snprintf(&pMessage[messageLen + prefixLen], 2, "\n");

    if (logger != NULL && logger->queue != NULL && level > AIO4C_LOG_LEVEL_FATAL) {
        if (!EnqueueDataItem(logger->queue, pMessage)) {
            _LogPrintMessage(logger, pMessage);
        }
    } else {
        _LogPrintMessage(logger, pMessage);
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
    FreeLock(&_logger.lock);
}

void LogBuffer(LogLevel level, Buffer* buffer) {
    int i = 0, j = 0;
    Thread* from = ThreadSelf();
    char c = '.';
    Logger* logger = &_logger;
    char* bufferText = NULL;
    char* currentBufferPointer = NULL;
    int bufferTextLen = 0;
    int bufferPos = 0;
    int printedLen = 0;

    if (logger != NULL && level > logger->level) {
        return;
    }

    bufferTextLen = AIO4C_LOG_BUFFER_SIZE * (buffer->limit - buffer->position) / 16;
    if ((buffer->limit - buffer->position) % 16) {
        bufferTextLen += AIO4C_LOG_BUFFER_SIZE;
    }
    bufferTextLen += AIO4C_LOG_PREFIX_SIZE + AIO4C_LOG_MESSAGE_SIZE;

    bufferText = aio4c_malloc(bufferTextLen);
    if (bufferText == NULL) {
        return;
    }

    currentBufferPointer = &bufferText[bufferPos];

    bufferPos += _LogPrefix(from, level, &currentBufferPointer);

    printedLen = snprintf(&bufferText[bufferPos], bufferTextLen - bufferPos, "dumping buffer %p [pos: %u, lim: %u, cap: %u]\n", (void*)buffer, buffer->position, buffer->limit, buffer->size);
    if (printedLen > 0) {
        bufferPos += printedLen;
    }

    if (buffer->limit >= 16) {
        for (i = buffer->position; i < buffer->limit - 16; i += 16) {
            currentBufferPointer = &bufferText[bufferPos];

            bufferPos += _LogPrefix(NULL, level, &currentBufferPointer);

            printedLen = snprintf(&bufferText[bufferPos], bufferTextLen - bufferPos, "0x%08x: ", i);
            if (printedLen > 0) {
                bufferPos += printedLen;
            }

            for (j = 0; j < 16; j ++) {
                printedLen = snprintf(&bufferText[bufferPos], bufferTextLen - bufferPos, "%02x ", (buffer->data[i + j] & 0xff));
                if (printedLen > 0) {
                    bufferPos += printedLen;
                }
            }

            for (j = 0; j < 16; j ++) {
                c = (char)(buffer->data[i + j] & 0xff);
                if (!isprint((int)c)) {
                    c = '.';
                }

                printedLen = snprintf(&bufferText[bufferPos], bufferTextLen - bufferPos, "%c", c);
                if (printedLen > 0) {
                    bufferPos += printedLen;
                }
            }

            printedLen = snprintf(&bufferText[bufferPos], bufferTextLen - bufferPos, "%c", '\n');
            if (printedLen > 0) {
                bufferPos += printedLen;
            }
        }
    }

    if ((buffer->limit - buffer->position) % 16) {
        i = buffer->limit - ((buffer->limit - buffer->position) % 16);

        currentBufferPointer = &bufferText[bufferPos];

        bufferPos += _LogPrefix(NULL, level, &currentBufferPointer);

        printedLen = snprintf(&bufferText[bufferPos], bufferTextLen - bufferPos, "0x%08x: ", i);
        if (printedLen > 0) {
            bufferPos += printedLen;
        }

        for (j = 0; j < buffer->limit - i; j++) {
            printedLen = snprintf(&bufferText[bufferPos], bufferTextLen - bufferPos, "%02x ", (buffer->data[i + j] & 0xff));
            if (printedLen > 0) {
                bufferPos += printedLen;
            }
        }

        for (; j < 16; j++) {
            printedLen = snprintf(&bufferText[bufferPos], bufferTextLen - bufferPos, "   ");
            if (printedLen > 0) {
                bufferPos += printedLen;
            }
        }

        for (j = 0; j < buffer->limit - i; j++) {
            c = (char)(buffer->data[i + j] & 0xff);
            if (!isprint((int)c)) {
                c = '.';
            }

            printedLen = snprintf(&bufferText[bufferPos], bufferTextLen - bufferPos, "%c", c);
            if (printedLen > 0) {
                bufferPos += printedLen;
            }
        }

        printedLen = snprintf(&bufferText[bufferPos], bufferTextLen - bufferPos, "%c", '\n');
    }

    if (logger != NULL && logger->queue != NULL) {
        if (!EnqueueDataItem(logger->queue, bufferText)) {
            _LogPrintMessage(logger, bufferText);
        }
    } else {
        _LogPrintMessage(logger, bufferText);
    }
}

