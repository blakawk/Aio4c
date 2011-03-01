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
    private long pClient;
    private int addressType;
    private String address;
    private short port;
    private int retries;
    private int retryInterval;
    private int bufferSize;
    private int logLevel;
    private String log;

    static {
        System.loadLibrary("aio4c");
    }

    native long Create();

    native void End();

    public Client(int addressType, String address, short port, int logLevel, String log, int retries, int retryInterval, int bufferSize) {
        this.addressType = addressType;
        this.address = address;
        this.port = port;
        this.retries = retries;
        this.retryInterval = retryInterval;
        this.bufferSize = bufferSize;
        this.logLevel = logLevel;
        this.log = log;

        this.pClient = Create();
    }

    private void onInit() {
        System.out.println("initialized");
    }

    private void onConnect() {
        System.out.println("connected");
    }

    private void onClose() {
        System.out.println("closed");
    }

    private void processData(byte[] data) {
        System.out.println("process data" + new String(data));
    }

    private int writeData(byte[] data) {
        System.out.println("write data");
        return 0;
    }

    public static void main(String[] args) {
        Client c = new Client(1, "localhost", (short)11111, 4, "client.log", 3, 3, 8192);
        c.End();
    }
}
