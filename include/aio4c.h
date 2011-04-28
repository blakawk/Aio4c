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
#ifndef __AIO4C_H__
#define __AIO4C_H__

#include <aio4c/alloc.h>
#include <aio4c/buffer.h>
#include <aio4c/client.h>
#include <aio4c/connection.h>
#include <aio4c/log.h>
#include <aio4c/server.h>
#include <aio4c/stats.h>

extern AIO4C_API void Aio4cUsage(void);

extern AIO4C_API void Aio4cInit(int argc, char* argv[]);

extern AIO4C_API void Aio4cEnd(void);

#endif
