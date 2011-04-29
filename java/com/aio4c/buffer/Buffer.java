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
     * Retrieves one <code>byte</code> at this buffer's position.
     *
     * Increments this buffer's position by 1 if there is enough data.
     *
     * @return
     *   The <code>byte</code> at this current buffer's position.
     * @throws com.aio4c.buffer.BufferUnderflowException
     *   When there is not enough data.
     */
    public native byte get() throws BufferUnderflowException;
    /**
     * Retrieves one <code>short</code> at this buffer's position.
     *
     * Increments this buffer's position by 2 if there is enough data.
     *
     * @return
     *   The <code>short</code> at this current buffer's position.
     * @throws com.aio4c.buffer.BufferUnderflowException
     *   When there is not enough data.
     */
    public native short getShort() throws BufferUnderflowException;
    /**
     * Retrieves one <code>int</code> at this buffer's position.
     *
     * Increments this buffer's position by 4 if there is enough data.
     *
     * @return
     *   The <code>int</code> at this current buffer's position.
     * @throws com.aio4c.buffer.BufferUnderflowException
     *   When there is not enough data.
     */
    public native int getInt() throws BufferUnderflowException;
    /**
     * Retrieves one <code>long</code> at this buffer's position.
     *
     * Increments this buffer's position by 8 if there is enough data.
     *
     * @return
     *   The <code>long</code> at this current buffer's position.
     * @throws com.aio4c.buffer.BufferUnderflowException
     *   When there is not enough data.
     */
    public native long getLong() throws BufferUnderflowException;
    /**
     * Retrieves one <code>float</code> at this buffer's position.
     *
     * Increments this buffer's position by 4 if there is enough data.
     *
     * @return
     *   The <code>float</code> at this current buffer's position.
     * @throws com.aio4c.buffer.BufferUnderflowException
     *   When there is not enough data.
     */
    public native float getFloat() throws BufferUnderflowException;
    /**
     * Retrieves one <code>double</code> at this buffer's position.
     *
     * Increments this buffer's position by 8 if there is enough data.
     *
     * @return
     *   The <code>double</code> at this current buffer's position.
     * @throws com.aio4c.buffer.BufferUnderflowException
     *   When there is not enough data.
     */
    public native double getDouble() throws BufferUnderflowException;
    /**
     * Retrieves one <code>String</code> at this buffer's position.
     *
     * Increments this buffer's position by {@link String#length()} plus one if
     * there is enough data.
     * 
     * If there is not enough data, there is two cases:
     * <ul>
     *   <li>there is remaining data in this buffer, but no terminating '\0',
     *   then, the returned <code>String</code> will contains the characters
     *   between this current buffer's position and limit (meaning that the
     *   terminating '\0' will be added automatically)</li>
     *   <li>there is no available data in the buffer, then an empty <code>
     *   String</code> is returned and this buffer's position and limit are
     *   left unchanged.</li>
     * </ul>
     * 
     * The <code>String</code> will be retrieved from the buffer considering
     * that the <code>String</code> was stored as a modified UTF-8, meaning
     * that if the <code>String</code> contained UTF-8 characters, they were
     * stored as 2-bytes, while simple ASCII characters were stored as 1-byte,
     * as well as terminating '\0'.
     * 
     * If you want to retrieve <code>String</code> stored in plain UTF-8 format,
     * use {@link Buffer#getWideString()}.
     *
     * @return
     *   The <code>String</code> at this buffer's position.
     */
    public native String getString();
    /**
     * Retrieves one <code>String</code> at this buffer's position.
     *
     * Increments this buffer's position by two times {@link String#length()}
     * plus two if there is enough data.
     * 
     * If there is not enough data, there is two cases:
     * <ul>
     *   <li>there is remaining data in this buffer, but no terminating '\0',
     *   then, the returned <code>String</code> will contains the characters
     *   between this current buffer's position and limit (meaning that the
     *   terminating '\0' will be added automatically)</li>
     *   <li>there is no available data in the buffer, then an empty <code>
     *   String</code> is returned and this buffer's position and limit are
     *   left unchanged.</li>
     * </ul>
     * 
     * The <code>String</code> will be retrieved from the buffer considering
     * that the <code>String</code> was stored in plain UTF-8 format, meaning
     * that all <code>String</code>'s characters were stored as 2-bytes, even
     * the terminating '\0'.
     * 
     * If you want to retrieve <code>String</code> stored in modified UTF-8
     * format, use {@link Buffer#getString()}.
     *
     * @return
     *   The <code>String</code> at this buffer's position.
     */
    public native String getWideString();
    /**
     * Puts one <code>byte</code> at this buffer's position.
     *
     * Increments this buffer's position by 1 if there is enough room.
     *
     * @param b
     *   The <code>byte</code> to put at this buffer's position.
     * @throws com.aio4c.buffer.BufferOverflowException
     *   When there is not enough room.
     */
    public native void put(byte b) throws BufferOverflowException;
    /**
     * Puts one <code>short</code> at this buffer's position.
     *
     * Increments this buffer's position by 2 if there is enough room.
     *
     * @param s
     *   The <code>short</code> to put at this buffer's position.
     * @throws com.aio4c.buffer.BufferOverflowException
     *   When there is not enough room.
     */
    public native void putShort(short s) throws BufferOverflowException;
    /**
     * Puts one <code>int</code> at this buffer's position.
     *
     * Increments this buffer's position by 4 if there is enough room.
     *
     * @param i
     *   The <code>int</code> to put at this buffer's position.
     * @throws com.aio4c.buffer.BufferOverflowException
     *   When there is not enough room.
     */
    public native void putInt(int i) throws BufferOverflowException;
    /**
     * Puts one <code>long</code> at this buffer's position.
     *
     * Increments this buffer's position by 8 if there is enough room.
     *
     * @param l
     *   The <code>long</code> to put at this buffer's position.
     * @throws com.aio4c.buffer.BufferOverflowException
     *   When there is not enough room.
     */
    public native void putLong(long l) throws BufferOverflowException;
    /**
     * Puts one <code>float</code> at this buffer's position.
     *
     * Increments this buffer's position by 4 if there is enough room.
     *
     * @param f
     *   The <code>float</code> to put at this buffer's position.
     * @throws com.aio4c.buffer.BufferOverflowException
     *   When there is not enough room.
     */
    public native void putFloat(float f) throws BufferOverflowException;
    /**
     * Puts one <code>double</code> at this buffer's position.
     *
     * Increments this buffer's position by 8 if there is enough room.
     *
     * @param d
     *   The <code>double</code> to put at this buffer's position.
     * @throws com.aio4c.buffer.BufferOverflowException
     *   When there is not enough room.
     */
    public native void putDouble(double d) throws BufferOverflowException;
    /**
     * Puts one <code>String</code> at this buffer's position.
     *
     * Increments this buffer's position by {@link String#length()} plus one if there is enough room.
     * The <code>String</code> will be stored in the buffer as a modified UTF-8 string, meaning that
     * if the <code>String</code> contains UTF-8 characters, they will be stored as 2-bytes, while
     * simple ASCII characters will be stored as 1-byte, as well as terminating '\0'.
     * 
     * If you want to store plain UTF-8 strings, use {@link Buffer#putWideString(String)}.
     *
     * @param s
     *   The <code>String</code> to put at this buffer's position.
     * @throws com.aio4c.buffer.BufferOverflowException
     *   When there is not enough room.
     */
    public native void putString(String s) throws BufferOverflowException;
    /**
     * Puts one <code>String</code> at this buffer's position.
     *
     * Increments this buffer's position by two times {@link String#length()}
     * plus two if there is enough room.
     * 
     * The <code>String</code> will be stored in the buffer as wide UTF-8
     * string, meaning that all <code>String</code>'s characters will be
     * stored as 2-bytes, even the terminating '\0'.
     * 
     * If you want to store modified UTF-8 strings, use
     * {@link Buffer#putString(String)} instead.
     *
     * @param s
     *   The <code>String</code> to put at this buffer's position.
     * @throws com.aio4c.buffer.BufferOverflowException
     *   When there is not enough room.
     */
    public native void putWideString(String s) throws BufferOverflowException;
    /**
     * Determines if this buffer has remaining data.
     * 
     * @return
     *   <code>true</code> when this buffer's position is less than this
     *   buffer's limit, else <code>false</code>.
     */
    public native boolean hasRemaining();
    /**
     * Retrieves this buffer's current position.
     * 
     * @return
     *   This buffer's current position.
     */
    public native int position();
    /**
     * Sets this buffer's position.
     * 
     * @param position
     *   The position to set.
     * @throws IllegalArgumentException
     *   When the position exceeds this buffer's current limit.
     */
    public native void position(int position) throws IllegalArgumentException;
    /**
     * Retrieves this buffer's current limit.
     * 
     * @return
     *  This buffer's current limit.
     */
    public native int limit();
    /**
     * Sets this buffer's limit.
     * 
     * @param limit
     *   The limit to set.
     * @throws IllegalArgumentException
     *   When the limit exceeds this buffer's current capacity.
     */
    public native void limit(int limit) throws IllegalArgumentException;
    /**
     * Retrieves this buffer's current capacity.
     * 
     * @return
     *   This buffer's current capacity.
     */
    public native int capacity();
    /**
     * Flips this buffer.
     * 
     * Sets this buffer's limit to its current position, and this buffer's
     * position to 0.
     */
    public native void flip();
    /**
     * Resets this buffer.
     * 
     * Fills this buffer's underlying data array with zeros, and then sets
     * this buffer's position to 0 and limit to its capacity. 
     */
    public native void reset();
    /**
     * Allocates a Buffer.
     * 
     * @param capacity
     *   The capacity of the allocated buffer.
     * @return
     *   The newly allocated buffer.
     * @throws OutOfMemoryError
     *   When there is not enough memory to allocate the capacity.
     */
    public static native Buffer allocate(int capacity) throws OutOfMemoryError;
    /**
     * Retrieves a <code>String</code> representation of this buffer.
     * 
     * The returned <code>String</code> is formatted as following:
     * <pre>
     * String.format("buffer %p [cap: %d, lim: %d, pos: %d]",
     *    System.getProperty("os.arch").equals("x86_64")?
     *      this.lPointer:this.iPointer,
     *    this.capacity(),
     *    this.limit(),
     *    this.position());
     * </pre>.
     * 
     * @see java.lang.Object#toString()
     */
    @Override
    public native String toString();
    /**
     * Pointer to the underlying buffer structure on 32 bits architectures.
     * Do not remove ! Used by JNI.
     */
    private int  iPointer = 0;
    /**
     * Pointer to the underlying buffer structure on 64 bits architectures.
     * Do not remove ! Used by JNI.
     */
    private long lPointer = 0;
    /**
     * Constructs a Buffer instance.
     * 
     * This constructor is defined as private because it will be called only
     * from the JNI interface, and must not be called anywhere else.
     */
    private Buffer() {
        // buffer initialization is performed in JNI
    };
}
