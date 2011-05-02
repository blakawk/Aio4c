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
package com.aio4c.log;

import java.util.Calendar;

/**
 * Dummy default logger that log messages to stderr, prefixing current date
 * to them.
 * 
 * @author blakawk
 * @see Logger
 */
public class DefaultLogger implements Logger {
    /**
     * Basic log output to stderr.
     * 
     * @param message
     *   The message to log.
     */
    public void log(String message) {
        System.out.print("[" + Calendar.getInstance().getTime().toString() + "] " + message);
    }
    @Override
    public void fatal(String message) {
        log("[fatal] " + message);
    }
    @Override
    public void error(String message) {
        log("[error] " + message);
    }
    @Override
    public void warn(String message) {
        log("[warn] " + message);
    }
    @Override
    public void info(String message) {
        log("[info] " + message);
    }
    @Override
    public void debug(String message) {
        log("[debug] " + message);
    }
}