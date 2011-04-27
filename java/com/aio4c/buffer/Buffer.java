/**
 * This file is part of Aio4c <http://aio4c.so>.
 *
 * Aio4c <http://aio4c.so> is free software: you
 * can  redistribute  it  and/or modify it under
 * the  terms  of the GNU General Public License
 * as published by the Free Software Foundation,
 * version 3 of the License.
 *
 * This  program is distributed in the hope that
 * it  will be useful, but WITHOUT ANY WARRANTY;
 * without   even   the   implied   warranty  of
 * MERCHANTABILITY  or  FITNESS FOR A PARTICULAR
 * PURPOSE.
 *
 * See  the  GNU General Public License for more
 * details.  You  should have received a copy of
 * the  GNU  General  Public  License along with
 * Aio4c    <http://aio4c.so>.   If   not,   see
 * <http://www.gnu.org/licenses/>.
 */
package com.aio4c.buffer;

/**
 * Class that represents a buffer used for I/O operations.
 *
 * @author blakawk
 * @see "aio4c/buffer.h"
 */
public class Buffer {
    /**
     * Retrieves a copy of this buffer's underlying byte array.
     *
     * As the returned array is only a copy, if modifications are made to it,
     * it must be sent back to the library using the setter.
     *
     * The array size and data matches this buffer's bytes between its position
     * and its limit (thus, it can be empty).
     *
     * @return
     *   A copy of this buffer's byte array.
     *
     * @see com.aio4c.buffer.Buffer#array(byte[])
     */
    public native byte[] array();
    /**
     * Sets the underlying buffer's byte array.
     *
     * If the array is greater than this buffer's remaining bytes, the supplementary
     * data is discarded.
     *
     * @param array
     *   Sets this buffer's bytes, updating this buffer's position.
     */
    public native void array(byte[] array);
    /**
     * Retrieves the <code>byte</code> at this buffer's position.
     *
     * Increments this buffer's position by 1 if there is remaining bytes.
     *
     * @return
     *   The <code>byte</code> at this current buffer's position.
     * @throws com.aio4c.buffer.BufferUnderflowException
     *   When this buffer's position exceeds this buffer's limit.
     */
    public native byte get() throws BufferUnderflowException;
    public native short getShort() throws BufferUnderflowException;
    public native int getInt() throws BufferUnderflowException;
    public native long getLong() throws BufferUnderflowException;
    public native float getFloat() throws BufferUnderflowException;
    public native double getDouble() throws BufferUnderflowException;
    public native String getString();
    public native void put(byte b) throws BufferOverflowException;
    public native void putShort(short s) throws BufferOverflowException;
    public native void putInt(int i) throws BufferOverflowException;
    public native void putLong(long l) throws BufferOverflowException;
    public native void putFloat(float f) throws BufferOverflowException;
    public native void putDouble(double d) throws BufferOverflowException;
    public native void putString(String s) throws BufferOverflowException;
    public native boolean hasRemaining();
    public native int position();
    public native void position(int position) throws IllegalArgumentException;
    public native int limit();
    public native void limit(int limit) throws IllegalArgumentException;
    public native void flip();
    public native int capacity();
    public native void reset();
    public static native Buffer allocate(int capacity) throws OutOfMemoryError;
    @Override
    public native String toString();
    /**
     * Pointer to the underlying buffer structure on 32 bits architectures.
     * Do not remove ! Used by JNI.
     */
    @SuppressWarnings("unused")
    private int  iPointer = 0;
    /**
     * Pointer to the underlying buffer structure on 64 bits architectures.
     * Do not remove ! Used by JNI.
     */
    @SuppressWarnings("unused")
    private long lPointer = 0;
    private Buffer() {};
}
