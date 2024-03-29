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
package com.aio4c.buffer;

/**
 * Exception raised when trying to retrieve data from a buffer with not enough
 * remaining data.
 *
 * @author blakawk
 * 
 * @see com.aio4c.buffer.Buffer
 * @see com.aio4c.buffer.BufferException
 */
public class BufferUnderflowException extends BufferException {
    /**
     * Used for serialization process.
     */
    private static final long serialVersionUID = -7224988735421974047L;
    /**
     * Constructs a BufferUnderflowException instance.
     * 
     * Does nothing except calling {@link BufferException#BufferException(Buffer)}.
     * 
     * @param buffer
     *   The buffer that caused this Exception.
     * 
     * @see BufferException#BufferException(Buffer)
     */
    public BufferUnderflowException(Buffer buffer) {
        super(buffer);
    }
}
