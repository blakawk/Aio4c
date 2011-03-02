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
#include <aio4c/log.h>

#include <aio4c/alloc.h>
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

static void _LogInit(Logger* logger) {
    Log(logger->thread, AIO4C_LOG_LEVEL_INFO, "logging is initialized");
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
    Log(logger->thread, AIO4C_LOG_LEVEL_INFO, "logging finished");

    _LogRun(logger);

    fclose(logger->file);
    logger->file = NULL;

    FreeQueue(&logger->queue);
}

void LogInit(Thread* parent, LogLevel level, char* filename) {
    _logger.level = level;
    _logger.thread = NULL;
    _logger.exiting = false;

    if (filename != NULL) {
        if ((_logger.file = fopen(filename, "a")) == NULL) {
            _logger.file = stderr;
            Log(parent, AIO4C_LOG_LEVEL_WARN, "open %s: %s, therefore logging will be performed on console instead", filename, strerror(errno));
        }
    } else {
        _logger.file = stderr;
    }

    if ((_logger.queue = NewQueue()) == NULL) {
        Log(parent, AIO4C_LOG_LEVEL_WARN, "cannot create log messages queue: %s, therefore no thread will be used", strerror(errno));
    } else {
        _logger.thread = NewThread("logger", (void(*)(void*))_LogInit, (aio4c_bool_t(*)(void*))_LogRun, (void(*)(void*))_LogExit, (void*)&_logger);
        if (_logger.thread == NULL) {
            Log(parent, AIO4C_LOG_LEVEL_WARN, "cannot create logging thread, logging may slow down performances");
        }
    }
}

static aio4c_size_t _LogPrefix(Thread* from, LogLevel level, char** message) {
    char* pMessage = NULL;
    char* levelString = "";
    struct timeval tv;
    struct tm* tm = NULL;
    aio4c_size_t prefixLen = 0, fromLen = 0;

    if ((pMessage = aio4c_malloc(AIO4C_MAX_LOG_MESSAGE_SIZE * sizeof(char))) == NULL) {
        *message = NULL;
        return 0;
    }

    memset(&tv, 0, sizeof(struct timeval));

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

    prefixLen = snprintf(pMessage, AIO4C_MAX_LOG_MESSAGE_SIZE, "[%02d:%02d:%02d.%03d %02d/%02d/%02d] [%s] ",
            tm->tm_hour, tm->tm_min, tm->tm_sec, (int)(tv.tv_usec / 1000), tm->tm_mday,
            tm->tm_mon + 1, tm->tm_year % 100, levelString);

    if (from == NULL) {
        from = ThreadSelf();
    }

    if (from != NULL) {
        fromLen = snprintf(&pMessage[prefixLen], AIO4C_MAX_LOG_MESSAGE_SIZE - prefixLen, "%s: ", from->name);
    }

    *message = pMessage;

    return prefixLen + fromLen;
}

void Log(Thread* from, LogLevel level, char* message, ...) {
    va_list args;
    Logger* logger = &_logger;
    char* pMessage = NULL;
    aio4c_size_t messageLen = 0, prefixLen = 0;

    if (logger != NULL && level > logger->level) {
        return;
    }

    prefixLen = _LogPrefix(from, level, &pMessage);
    if (pMessage == NULL) {
        return;
    }

    va_start(args, message);
    messageLen += vsnprintf(&pMessage[prefixLen], AIO4C_MAX_LOG_MESSAGE_SIZE - prefixLen, message, args);
    va_end(args);

    if (messageLen >= AIO4C_MAX_LOG_MESSAGE_SIZE - prefixLen) {
        if ((pMessage = aio4c_realloc(pMessage, prefixLen + messageLen + 2)) == NULL) {
            aio4c_free(pMessage);
            return;
        }

        va_start(args, message);
        messageLen = vsnprintf(&pMessage[prefixLen], messageLen - prefixLen, message, args);
        va_end(args);
    }

    snprintf(&pMessage[messageLen + prefixLen], 2, "\n");

    if (logger != NULL && logger->queue != NULL) {

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
            FreeThread(&logger->thread);
        }
    }
}

void LogBuffer(Thread* from, LogLevel level, Buffer* buffer) {
    int i = 0, j = 0;
    char c = '.';
    Logger* logger = &_logger;
    aio4c_size_t prefixLen = 0;
    char* message = NULL;

    if (logger != NULL && level > logger->level) {
        return;
    }

    Log(from, level, "dumping buffer %p [pos: %u, lim: %u, cap: %u]", (void*)buffer, buffer->position, buffer->limit, buffer->size);

    if (buffer->limit >= 16) {
        for (i = buffer->position; i < buffer->limit; i += 16) {
            prefixLen = _LogPrefix(from, level, &message);
            prefixLen += snprintf(&message[prefixLen], AIO4C_MAX_LOG_MESSAGE_SIZE - prefixLen, "0x%08x: ", i);

            for (j = 0; j < 16; j ++) {
                prefixLen += snprintf(&message[prefixLen], AIO4C_MAX_LOG_MESSAGE_SIZE - prefixLen, "%02x ", (buffer->data[i + j] & 0xff));
            }

            for (j = 0; j < 16; j ++) {
                c = (char)(buffer->data[i + j] & 0xff);
                if (!isprint((int)c)) {
                    c = '.';
                }
                prefixLen += snprintf(&message[prefixLen], AIO4C_MAX_LOG_MESSAGE_SIZE - prefixLen, "%c", c);
            }
            snprintf(&message[prefixLen], AIO4C_MAX_LOG_MESSAGE_SIZE - prefixLen, "\n");
            if (logger != NULL && logger->queue != NULL) {
                if (!EnqueueDataItem(logger->queue, message)) {
                    _LogPrintMessage(logger, message);
                }
            } else {
                _LogPrintMessage(logger, message);
            }
        }
    }

    if (i == buffer->limit) {
        return;
    }

    prefixLen = _LogPrefix(from, level, &message);
    prefixLen += snprintf(&message[prefixLen], AIO4C_MAX_LOG_MESSAGE_SIZE - prefixLen, "0x%08x: ", i);

    for (j = 0; j < buffer->limit - i; j++) {
        prefixLen += snprintf(&message[prefixLen], AIO4C_MAX_LOG_MESSAGE_SIZE - prefixLen, "%02x ", (buffer->data[i + j] & 0xff));
    }

    for (; j < 16; j++) {
        prefixLen += snprintf(&message[prefixLen], AIO4C_MAX_LOG_MESSAGE_SIZE - prefixLen, "   ");
    }

    for (j = 0; j < buffer->limit - i; j++) {
        c = (char)(buffer->data[i + j] & 0xff);
        if (!isprint((int)c)) {
            c = '.';
        }
        prefixLen += snprintf(&message[prefixLen], AIO4C_MAX_LOG_MESSAGE_SIZE - prefixLen, "%c", c);
    }

    snprintf(&message[prefixLen], AIO4C_MAX_LOG_MESSAGE_SIZE - prefixLen, "\n");

    if (logger != NULL && logger->queue != NULL) {
        if (!EnqueueDataItem(logger->queue, message)) {
            _LogPrintMessage(logger, message);
        }
    } else {
        _LogPrintMessage(logger, message);
    }
}

