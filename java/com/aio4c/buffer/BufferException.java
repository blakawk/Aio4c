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
package com.aio4c.buffer;

/**
 * Buffer exceptions superclass.
 *
 * All exceptions raised by Buffer's methods should be children of this class.
 * 
 * @author blakawk
 * 
 * @see com.aio4c.buffer.Buffer
 */
public abstract class BufferException extends Exception {
    /**
     * Used for serialization process.
     */
    private static final long serialVersionUID = -8203712034321914597L;
    /**
     * The Buffer which caused this exception.
     */
    private Buffer buffer;
    /**
     * Constructs a new BufferException.
     * 
     * @param buffer
     *   The buffer that triggered this Exception.
     */
    public BufferException(Buffer buffer) {
        super();
        this.buffer = buffer;
    }
    /*
     * @see java.lang.Throwable#getMessage()
     */
    @Override
    public String getMessage() {
        StringBuilder sb = new StringBuilder();
        sb.append("in ");
        if (this.buffer != null) {
            sb.append(this.buffer.toString());
        } else {
            sb.append("<unknown buffer>");
        }
        return sb.toString();
    }
}
