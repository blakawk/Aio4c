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

import com.aio4c.buffer.Buffer;

public abstract class Connection {
    /**
     * Pointer to the underlying connection structure on 32 bits architectures.
     * Do not remove ! Used by JNI.
     */
    @SuppressWarnings("unused")
    private int  iPointer = 0;
    /**
     * Pointer to the underlying connection structure on 64 bits architectures.
     * Do not remove ! Used by JNI.
     */
    @SuppressWarnings("unused")
    private long lPointer = 0;
    public void onInit() {};
    public void onConnect() {};
    public void onRead(@SuppressWarnings("unused") Buffer data) {};
    public void onWrite(@SuppressWarnings("unused") Buffer data) {};
    public void onClose() {};
    public native void enableWriteInterest();
    public native void close(boolean force);
    public native boolean closing();
    @Override
    public native String toString();
    public Connection() {};
}
