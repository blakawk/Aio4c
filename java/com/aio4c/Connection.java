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

import com.aio4c.buffer.Buffer;

/**
 * Represents a TCP Connection. 
 *
 * @author blakawk
 */
public abstract class Connection {
    /**
     * Pointer to the underlying connection structure on 32 bits architectures.
     * Do not remove ! Used by JNI.
     */
    private int  iPointer = 0;
    /**
     * Pointer to the underlying connection structure on 64 bits architectures.
     * Do not remove ! Used by JNI.
     */
    private long lPointer = 0;
    /**
     * Method called when AIO4C_INIT_EVENT occurs.
     * 
     * This method is called when the Connection is initialized, meaning that
     * data structure has been allocated and is ready to be used.
     * 
     * Override this method to initialize your own implementation.
     * 
     * @see "aio4c/event.h#AIO4C_INIT_EVENT"
     */
    public void onInit() {
        // should be overridden if needed
    };
    /**
     * Method called when AIO4C_CONNECTED_EVENT occurs.
     * 
     * This method is called when the Connection switches to 
     * AIO4C_CONNECTION_STATE_CONNECTED state. This occurs when:
     * <ul>
     *   <li>for Client, the connection has been established with the server,</li>
     *   <li>for Server, a new connection was accepted.</li>
     * </ul>
     * 
     * Uses it to update some state variable of your own implementation.
     * 
     * @see "aio4c/event.h#AIO4C_CONNECTED_EVENT"
     * @see "aio4c/connection.h#AIO4C_CONNECTION_STATE_CONNECTED"
     */
    public void onConnect() {
        // should be overridden if needed
    };
    /**
     * Method called on AIO4C_INBOUND_DATA_EVENT.
     * 
     * This method is called when there is data received from the network
     * on the Connection.
     * 
     * @param data
     *   The Buffer handling received data.
     * 
     * @see com.aio4c.buffer.Buffer
     * @see "aio4c/event.h#AIO4C_INBOUND_DATA_EVENT"
     */
    public void onRead(@SuppressWarnings("unused") Buffer data) {
        // should be overridden if needed
    };
    /**
     * Method called on AIO4C_WRITE_EVENT.
     * 
     * This method is called for the user space to provide the data to send
     * on the Connection.
     * 
     * @param data
     *   The Buffer to put data to send in.
     *   
     * @see com.aio4c.buffer.Buffer
     * @see "aio4c/event.h#AIO4C_WRITE_EVENT"
     */
    public void onWrite(@SuppressWarnings("unused") Buffer data) {
        // should be overridden if needed
    };
    /**
     * Method called on AIO4C_CLOSE_EVENT.
     * 
     * This method is called when this Connection is closed for any reason
     * (graceful or unexpected disconnection from remote). 
     */
    public void onClose() {
        // should be overridden if needed
    };
    /**
     * This method indicates that this Connection has data ready to be sent.
     * The underlying library will then call {@link Connection#onWrite(Buffer)}
     * for the user to write the data to be sent.
     * 
     * The {@link Connection#onWrite(Buffer)} method will be called the same
     * number of times this method is called, so either use a FIFO to store
     * outgoing data, or override this method using a lock that will be
     * unlocked when {@link Connection#onWrite(Buffer)} is called. 
     */
    public native void enableWriteInterest();
    /**
     * Closes this Connection.
     * 
     * When this Connection should be closed gracefully, reading and writing
     * on this Connection will be disabled, and output buffers will be sent
     * over the network, then, the connection will be closed using normal
     * TCP closing sequence FIN/FIN-ACK/ACK.
     * 
     * In the case the Connection should be closed "now", reading and writing
     * will be disabled, and the connection will be closed with a simple
     * RST TCP closing sequence.
     * 
     * @param force
     *   <code>true</code> if this Connection should be closed "now",
     *   discarding any remaining outgoing data, <code>false</code> if this
     *   Connection should be gracefully closed.
     */
    public native void close(boolean force);
    /**
     * Determines if this Connection is pending close.
     * 
     * @return
     *   <code>true</code> if this Connection wait for close,
     *   <code>false</code> in the other case.
     */
    public native boolean closing();
    /**
     * Returns a <code>String</code> representation of this Connection.
     * 
     * If this is not overridden, the <code>String</code> will be of following
     * format: {@literal <host name>:<port number>}. The {@literal <host name>}
     * format will be of following format depending on {@link AddressType}:
     * <ul>
     *   <li>{@link AddressType#IPV4}: IPv4 in dot notation,</li>
     *   <li>{@link AddressType#IPV6}: IPv6 in dotted-quad notation, enclosed in 
     *   brackets.</li>
     * </ul>
     * 
     * @return
     *   This Connection's <code>String</code> representation
     */
    @Override
    public native String toString();
    /**
     * Constructs a new Connection instance.
     * 
     * This constructor must be called from {@link ConnectionFactory#create()}.
     */
    public Connection() {
        // should be overridden if needed
    };
}
