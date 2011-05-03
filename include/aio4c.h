/*
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
/**
 * @file aio4c.h
 * @brief Exposes Aio4c API and init/cleanup functions.
 *
 * @author blakawk
 *
 * @see aio4c/buffer.h
 * @see aio4c/client.h
 * @see aio4c/connection.h
 * @see aio4c/log.h
 * @see aio4c/server.h
 * @see aio4c/stats.h
 */
/**
 * @def __AIO4C_H__
 * @brief Defined when aio4c.h has been included.
 *
 * @see aio4c.h
 */
#ifndef __AIO4C_H__
#define __AIO4C_H__

#include <aio4c/buffer.h>
#include <aio4c/client.h>
#include <aio4c/connection.h>
#include <aio4c/log.h>
#include <aio4c/server.h>
#include <aio4c/stats.h>

/**
 * @fn void Aio4cUsage()
 * @brief Displays library options and help message.
 *
 * The help message is displayed on stderr.
 */
extern AIO4C_API void Aio4cUsage(void);

/**
 * @fn void Aio4cInit(int,char*[],void(*)(void*,LogLevel,char*),void*)
 * @brief Initializes the library.
 *
 * This function should be called before any other call to a library function.
 * It initializes internal library structures and submodules, and parses
 * command line arguments for library options.
 *
 * @param argc
 *   The size of argv array.
 * @param argv
 *   The command line arguments.
 * @param loghandler
 *   The callback used to log library messages.
 * @param logger
 *   The data passed as argument to the log callback.
 */
extern AIO4C_API void Aio4cInit(int argc, char* argv[], void (*loghandler)(void*,LogLevel,char*), void* logger);

/**
 * @fn void Aio4cEnd()
 * @brief Frees all resources used by the library.
 */
extern AIO4C_API void Aio4cEnd(void);

#endif /* __AIO4C_H__ */
