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
package com.aio4c.log;

/**
 * Class that allows to use aio4c default logger from Java.
 * 
 * @author blakawk
 */
public class Log {
    /**
     * Matches LogLevel values from aio4c/log.h module.
     * 
     * @author blakawk
     * @see "aio4c/log.h#LogLevel"
     */
    public static enum Level {
        /**
         * @see "aio4c/log.h#AIO4C_LOG_LEVEL_FATAL" 
         */
        FATAL(0),
        /**
         * @see "aio4c/log.h#AIO4C_LOG_LEVEL_ERROR"
         */
        ERROR(1),
        /**
         * @see "aio4c/log.h#AIO4C_LOG_LEVEL_WARN"
         */
        WARN(2),
        /**
         * @see "aio4c/log.h#AIO4C_LOG_LEVEL_INFO"
         */
        INFO(3),
        /**
         * @see "aio4c/log.h#AIO4C_LOG_LEVEL_DEBUG"
         */
        DEBUG(4);
        /**
         * The value of this Level.
         */
        public int value = 0;
        /**
         * Constructs a Level instance. 
         *
         * @param value
         *   The value of this Level.
         */
        private Level(int value) {
            this.value = value;
        }
    }
    /**
     * Logs a level's message using aio4c/log.h#Log(LogLevel,char*).
     *  
     * @param level
     *   The message's log level.
     * @param message
     *   The message to log.
     */
    public static native void log(Level level, String message);
    /**
     * Formats and logs a fatal error message.
     * 
     * @param format
     *   The format string.
     * @param args
     *   The format string arguments, if any.
     */
    public static void fatal(String format, Object... args) {
        Log.log(Level.FATAL, String.format(format, args));
    }
    /**
     * Formats and logs an error message.
     * 
     * @param format
     *   The format string.
     * @param args
     *   The format string arguments, if any.
     */
    public static void error(String format, Object... args) {
        Log.log(Level.ERROR, String.format(format, args));
    }
    /**
     * Formats and logs a warning message.
     * 
     * @param format
     *   The format string.
     * @param args
     *   The format string arguments, if any.
     */
    public static void warn(String format, Object... args) {
        Log.log(Level.WARN, String.format(format, args));
    }
    /**
     * Formats and logs an informative message.
     * 
     * @param format
     *   The format string.
     * @param args
     *   The format string arguments, if any.
     */
    public static void info(String format, Object... args) {
        Log.log(Level.INFO, String.format(format, args));
    }
    /**
     * Formats and logs a message for debugging purpose.
     * 
     * @param format
     *   The format string.
     * @param args
     *   The format string arguments, if any.
     */
    public static void debug(String format, Object... args) {
        Log.log(Level.DEBUG, String.format(format, args));
    }
}
