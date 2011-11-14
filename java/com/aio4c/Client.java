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
package com.aio4c;

/**
 * Class that represents a Client.
 *
 * @author blakawk
 * @see "aio4c/client.h"
 */
public class Client {
    /**
     * Pointer to the underlying client structure on 32 bits architectures.
     * Do not remove ! Used by JNI.
     */
    private int  iPointer = 0;
    /**
     * Pointer to the underlying client structure on 64 bits architectures.
     * Do not remove ! Used by JNI.
     */
    private long lPointer = 0;
    /**
     * Initializes this client.
     * 
     * @param config
     *   This Client configuration.
     * @param factory
     *   The factory that will be used to create Connection instances.
     *
     * @see com.aio4c.ClientConfig
     * @see com.aio4c.Connection
     * @see com.aio4c.ConnectionFactory
     */
    private native void initialize(ClientConfig config, ConnectionFactory factory);
    /**
     * Starts this client.
     * 
     * @return
     *   <code>true</code> if this Client was successfully started,
     *   <code>false</code> in any other case.
     */
    public native boolean start();
    /**
     * Joins this client.
     * 
     * Waits for the Client to terminate. The Client will terminate only if it exceeds connection retry count.
     * 
     * @see com.aio4c.ClientConfig#retries
     */
    public native void join();
    /**
     * Constructs a Client instance.
     * 
     * @param config
     *   The ClientConfig to use to configure this Client.
     * @param factory
     *   The ConnectionFactory to use to create Connection instances. 
     */
    public Client(ClientConfig config, ConnectionFactory factory) {
        initialize(config, factory);
    }
}
