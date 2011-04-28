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
package com.aio4c;

public class Log {
    public static enum Level {
        FATAL(0),
        ERROR(1),
        WARN(2),
        INFO(3),
        DEBUG(4);

        public int value;

        private Level(int value) {
            this.value = value;
        }
    }

    private static native void log(Log.Level level, String message);
    public static void fatal(String format, Object... args) { log(Level.FATAL, String.format(format, args)); }
    public static void error(String format, Object... args) { log(Level.ERROR, String.format(format, args)); }
    public static void warn(String format, Object... args) { log(Level.WARN, String.format(format, args)); }
    public static void info(String format, Object... args) { log(Level.INFO, String.format(format, args)); }
    public static void debug(String format, Object... args) { log(Level.DEBUG, String.format(format, args)); }
}

