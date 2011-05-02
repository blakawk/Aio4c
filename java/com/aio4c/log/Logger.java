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
 * Interface used to handle log messages generated by the library.
 * 
 * Implement it and pass the implementation instance to 
 * {@link com.aio4c.Aio4c#init(String[],Logger)}.
 *
 * @author blakawk
 */
public interface Logger {
    /**
     * Will be called to log a fatal error.
     * 
     * @param message
     *   The message to log.
     */
    public void fatal(String message);
    /**
     * Will be called to log an error message.
     * 
     * @param message
     *   The message to log.
     */
    public void error(String message);
    /**
     * Will be called to log a warning message.
     * 
     * @param message
     *   The message to log.
     */
    public void warn(String message);
    /**
     * Will be called to log an informational message.
     * 
     * @param message
     *   The message to log.
     */
    public void info(String message);
    /**
     * Will be called to log a message for debugging purpose.
     * 
     * @param message
     *   The message to log.
     */
    public void debug(String message);
}
