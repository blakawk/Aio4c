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

public class Client {
    /**
     * Pointer to the underlying client structure on 32 bits architectures.
     * Do not remove ! Used by JNI.
     */
    @SuppressWarnings("unused")
    private int  iPointer = 0;
    /**
     * Pointer to the underlying client structure on 64 bits architectures.
     * Do not remove ! Used by JNI.
     */
    @SuppressWarnings("unused")
    private long lPointer = 0;
    private native void initialize(ClientConfig config, ConnectionFactory factory);
    public native void join();
    public Client(ClientConfig config, ConnectionFactory factory) {
        initialize(config, factory);
    }
}