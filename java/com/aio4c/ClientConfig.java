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
 * Represents a Client configuration.
 *
 * @author blakawk
 * 
 * @see com.aio4c.Client
 */
public class ClientConfig {
    /**
     * The Client identifier (used only for debug purpose).
     */
    public int clientId      = 0;
    /**
     * The kind of address to connect to.
     * 
     * @see com.aio4c.AddressType
     */
    public AddressType type  = AddressType.IPV4;
    /**
     * The address or host name the Client will connect to.
     */
    public String address    = "localhost";
    /**
     * The TCP port the Client will connect to.
     */
    public short port        = 8080;
    /**
     * The maximum number of retries when the Client loose Connection with the
     * Server unexpectedly.
     */
    public int retries       = 3;
    /**
     * The interval in seconds between two retries.
     */
    public int retryInterval = 30;
    /**
     * The Buffer capacity to use for network operations.
     * 
     * @see com.aio4c.buffer.Buffer#allocate(int)
     */
    public int bufferSize    = 8192;
}
