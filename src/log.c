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

static Logger* _logger = NULL;

static LogMessageQueue* _NewLogMessageQueue(void) {
    LogMessageQueue* queue = NULL;

    if ((queue = malloc(sizeof(LogMessageQueue))) == NULL) {
        return NULL;
    }

    queue->queue = NULL;
    queue->itemsCount = 0;
    queue->maxItems = 0;
    queue->lock = NewLock();
    queue->condition = NewCondition();

    return queue;
}

static aio4c_bool_t _LogMessageDequeue(Logger* logger, LogMessageQueue* queue, char** message) {
    int i = 0;
    aio4c_bool_t more = true;

    while (queue->itemsCount <= 0) {
        WaitCondition(logger->thread, queue->condition, queue->lock);
    }

    *message = queue->queue[0];
    for (i = 0; i < queue->maxItems - 1; i++) {
        queue->queue[i] = queue->queue[i+1];
    }
    queue->queue[i] = NULL;
    queue->itemsCount--;

    more = queue->itemsCount > 0;

    return more;
}

static void _LogPrintMessage(Logger* logger, char* message) {
    aio4c_file_t* log = NULL;

    if (message == NULL) {
        return;
    }

    if (logger != NULL && logger->file != NULL) {
        log = logger->file;
    } else {
        log = stderr;
    }

    fprintf(log, "%s", message);
    fflush(log);
    free(message);
}

static void _FreeLogMessageQueue(LogMessageQueue** queue) {
    LogMessageQueue* pQueue = NULL;
    int i = 0;

    if (queue != NULL && (pQueue = *queue) != NULL) {
        if (pQueue->queue != NULL) {
            for (i = 0; i < pQueue->maxItems; i++) {
                if (pQueue->queue[i] != NULL) {
                    free(pQueue->queue[i]);
                    pQueue->queue[i] = NULL;
                }
            }
            free(pQueue->queue);
            pQueue->queue = NULL;
            pQueue->maxItems = 0;
            pQueue->itemsCount = 0;
        }

        if (pQueue->lock != NULL) {
            FreeLock(&pQueue->lock);
        }

        if (pQueue->condition != NULL) {
            FreeCondition(&pQueue->condition);
        }

        free(pQueue);
        *queue = NULL;
    }
}

static void _LogMessageQueue(Thread* client, Logger* logger, char* message) {
    LogMessageQueue* queue = NULL;

    if (logger == NULL || (queue = logger->queue) == NULL) {
        _LogPrintMessage(logger, message);
        return;
    }

    TakeLock(client, queue->lock);

    if (queue->itemsCount == queue->maxItems) {
        if ((queue->queue = realloc(queue->queue, (queue->maxItems + 1) * sizeof(char*))) == NULL) {
            return;
        }
        queue->maxItems++;
    }

    queue->queue[queue->itemsCount++] = message;

    NotifyCondition(client, queue->condition, queue->lock);

    ReleaseLock(queue->lock);
}

static void _LogInit(Logger* logger) {
    Log(logger->thread, INFO, "logging is initialized");
}

static aio4c_bool_t _LogRun(Logger* logger) {
    char* message = NULL;

    TakeLock(logger->thread, logger->queue->lock);

    while (_LogMessageDequeue(logger, logger->queue, &message)) {
        _LogPrintMessage(logger, message);
        message = NULL;
    }

    _LogPrintMessage(logger, message);

    ReleaseLock(logger->queue->lock);

    return true;
}

static void _FreeLogger(Logger** logger) {
    Logger* pLogger = NULL;

    if (logger != NULL && (pLogger = *logger) != NULL) {
        if (pLogger->queue != NULL) {
            _FreeLogMessageQueue(&pLogger->queue);
        }

        if (pLogger->file != NULL) {
            fclose(pLogger->file);
            pLogger->file = NULL;
        }

        free(pLogger);
        *logger = NULL;
    }
}

static void _LogExit(Logger* logger) {
    ReleaseLock(logger->queue->lock);
    Log(logger->thread, INFO, "logging finished");
    _LogRun(logger);
    _FreeLogger(&logger);
}

void LogInit(Thread* parent, LogLevel level, char* filename) {
    if ((_logger = malloc(sizeof(Logger))) == NULL) {
        perror("FATAL ERROR: logger initialization failed");
        exit (EXIT_FAILURE);
    }

    _logger->level = level;
    _logger->thread = NULL;

    if (filename != NULL) {
        if ((_logger->file = fopen(filename, "a")) == NULL) {
            _logger->file = stderr;
            Log(parent, WARN, "open %s: %s, therefore logging will be performed on console instead", filename, strerror(errno));
        }
    } else {
        _logger->file = stderr;
    }

    if ((_logger->queue = _NewLogMessageQueue()) == NULL) {
        Log(parent, WARN, "cannot create log messages queue: %s, therefore no thread will be used", strerror(errno));
    } else {
        _logger->thread = NewThread(parent, "logger", (void(*)(void*))_LogInit, (aio4c_bool_t(*)(void*))_LogRun, (void(*)(void*))_LogExit, (void*)_logger);
        if (_logger->thread == NULL) {
            Log(parent, WARN, "cannot create logging thread, logging may slow down performances");
        }
    }
}

static aio4c_size_t _LogPrefix(Thread* from, LogLevel level, char** message) {
    char* pMessage = NULL;
    char* levelString = "";
    struct timeval tv;
    struct tm* tm = NULL;
    aio4c_size_t prefixLen = 0, fromLen = 0;

    if ((pMessage = malloc(MAX_LOG_MESSAGE_SIZE * sizeof(char))) == NULL) {
        *message = NULL;
        return 0;
    }

    memset(&tv, 0, sizeof(struct timeval));

    gettimeofday(&tv, NULL);
    tm = localtime((time_t*)&tv.tv_sec);

    switch(level) {
        case INFO:
            levelString = "info";
            break;
        case WARN:
            levelString = "warn";
            break;
        case ERROR:
            levelString = "error";
            break;
        case FATAL:
            levelString = "FATAL";
            break;
        case DEBUG:
            levelString = "debug";
            break;
        default:
            break;
    }

    prefixLen = snprintf(pMessage, MAX_LOG_MESSAGE_SIZE, "[%02d:%02d:%02d.%03d %02d/%02d/%02d] [%s] ",
            tm->tm_hour, tm->tm_min, tm->tm_sec, (int)(tv.tv_usec / 1000), tm->tm_mday,
            tm->tm_mon + 1, tm->tm_year % 100, levelString);

    if (from != NULL) {
        fromLen = snprintf(&pMessage[prefixLen], MAX_LOG_MESSAGE_SIZE - prefixLen, "%s: ", from->name);
    }

    *message = pMessage;

    return prefixLen + fromLen;
}

void Log(Thread* from, LogLevel level, char* message, ...) {
    va_list args;
    Logger* logger = _logger;
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
    messageLen += vsnprintf(&pMessage[prefixLen], MAX_LOG_MESSAGE_SIZE - prefixLen, message, args);
    va_end(args);

    if (messageLen >= MAX_LOG_MESSAGE_SIZE - prefixLen) {
        if ((pMessage = realloc(pMessage, prefixLen + messageLen + 2)) == NULL) {
            free(pMessage);
            return;
        }

        va_start(args, message);
        messageLen = vsnprintf(&pMessage[prefixLen], messageLen - prefixLen, message, args);
        va_end(args);
    }

    snprintf(&pMessage[messageLen + prefixLen], 2, "\n");

    if (logger != NULL && logger->thread != NULL) {
        _LogMessageQueue(from, logger, pMessage);
    } else {
        _LogPrintMessage(logger, pMessage);
    }
}

void LogEnd(Thread* from) {
    Thread* thread = NULL;
    if (_logger != NULL) {
        if ((thread = _logger->thread) != NULL) {
            ThreadCancel(from, thread, true);
            ThreadJoin(from, thread);
            FreeThread(from, &thread);
        } else {
            if (_logger->file != NULL) {
                fclose(_logger->file);
                _logger->file = NULL;
            }

            _FreeLogger(&_logger);
        }
    }
}

void LogBuffer(Thread* from, LogLevel level, Buffer* buffer) {
    int i = 0, j = 0;
    char c = '.';
    Logger* logger = _logger;
    aio4c_size_t prefixLen = 0;
    char* message = NULL;

    if (logger != NULL && level > logger->level) {
        return;
    }

    Log(from, level, "dumping buffer %p [pos: %u, lim: %u, cap: %u]", buffer, buffer->position, buffer->limit, buffer->size);

    if (buffer->limit >= 16) {
        for (i = buffer->position; i < buffer->limit; i += 16) {
            prefixLen = _LogPrefix(from, level, &message);
            prefixLen += snprintf(&message[prefixLen], MAX_LOG_MESSAGE_SIZE - prefixLen, "0x%08x: ", i);

            for (j = 0; j < 16; j ++) {
                prefixLen += snprintf(&message[prefixLen], MAX_LOG_MESSAGE_SIZE - prefixLen, "%02x ", (buffer->data[i + j] & 0xff));
            }

            for (j = 0; j < 16; j ++) {
                c = (char)(buffer->data[i + j] & 0xff);
                if (!isprint(c)) {
                    c = '.';
                }
                prefixLen += snprintf(&message[prefixLen], MAX_LOG_MESSAGE_SIZE - prefixLen, "%c", c);
            }
            snprintf(&message[prefixLen], MAX_LOG_MESSAGE_SIZE - prefixLen, "\n");
            if (logger != NULL && logger->thread != NULL) {
                _LogMessageQueue(from, logger, message);
            } else {
                _LogPrintMessage(logger, message);
            }
        }
    }

    if (i == buffer->limit) {
        return;
    }

    prefixLen = _LogPrefix(from, level, &message);
    prefixLen += snprintf(&message[prefixLen], MAX_LOG_MESSAGE_SIZE - prefixLen, "0x%08x: ", i);

    for (j = 0; j < buffer->limit - i; j++) {
        prefixLen += snprintf(&message[prefixLen], MAX_LOG_MESSAGE_SIZE - prefixLen, "%02x ", (buffer->data[i + j] & 0xff));
    }

    for (; j < 16; j++) {
        prefixLen += snprintf(&message[prefixLen], MAX_LOG_MESSAGE_SIZE - prefixLen, "   ");
    }

    for (j = 0; j < buffer->limit - i; j++) {
        c = (char)(buffer->data[i + j] & 0xff);
        if (!isprint(c)) {
            c = '.';
        }
        prefixLen += snprintf(&message[prefixLen], MAX_LOG_MESSAGE_SIZE - prefixLen, "%c", c);
    }

    snprintf(&message[prefixLen], MAX_LOG_MESSAGE_SIZE - prefixLen, "\n");

    if (logger != NULL && logger->thread != NULL) {
        _LogMessageQueue(from, logger, message);
    } else {
        _LogPrintMessage(logger, message);
    }
}

