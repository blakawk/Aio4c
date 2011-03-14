/**
 * Copyright © 2011 blakawk <blakawk@gentooist.com>
 * All rights reserved.  Released under GPLv3 License.
 *
 * This program is free software: you can redistribute
 * it  and/or  modify  it under  the  terms of the GNU.
 * General  Public  License  as  published by the Free
 * Software Foundation, version 3 of the License.
 *
 * This  program  is  distributed  in the hope that it
 * will be useful, but  WITHOUT  ANY WARRANTY; without
 * even  the  implied  warranty  of MERCHANTABILITY or
 * FITNESS FOR A PARTICULAR PURPOSE.
 *
 * See   the  GNU  General  Public  License  for  more
 * details. You should have received a copy of the GNU
 * General Public License along with this program.  If
 * not, see <http://www.gnu.org/licenses/>.
 **/
package com.aio4c;

public class Buffer {
    public native byte[] array();
    public native byte get();
    public native short getShort();
    public native int getInt();
    public native long getLong();
    public native float getFloat();
    public native double getDouble();
    public native String getString();
    public native void put(byte b);
    public native void putShort(short s);
    public native void putInt(int i);
    public native void putLong(long l);
    public native void putFloat(float f);
    public native void putDouble(double d);
    public native void putString(String s);
    private int  iPointer;
    private long lPointer;
    private Buffer() {};
}
