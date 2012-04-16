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
import com.aio4c.Aio4c;
import com.aio4c.ClientConfig;
import com.aio4c.Connection;
import com.aio4c.ConnectionFactory;
import com.aio4c.buffer.Buffer;
import com.aio4c.buffer.BufferOverflowException;
import com.aio4c.log.Log;
import com.aio4c.log.DefaultLogger;

public class Client {
    public static void main(String[] args) {
        Aio4c.init(args, new DefaultLogger());

        com.aio4c.Client c = new com.aio4c.Client(new ClientConfig(), new ConnectionFactory () {
            @Override
            public Connection create() {
                return new Connection () {
                    @Override
                    public void onInit() {
                        Log.debug("[" + this + "] init from java !");
                    }
                    @Override
                    public void onClose() {
                        Log.debug("[" + this + "] close from java !");
                    }
                    @Override
                    public void onConnect() {
                        Log.debug("[" + this + "] connect from java !");
                    }
                    @Override
                    public void onRead(Buffer data) {
                        while (data.hasRemaining()) {
                            String str = data.getString();
                            Log.debug("[" + this + "] received data "+data+" from java: '" + str + "' !");
                            if (str.trim().equals("QUIT")) {
                                close(false);
                            }
                            enableWriteInterest();
                        }
                    }
                    @Override
                    public void onWrite(Buffer data) {
                        String str = "GOODBYE";
                        try {
                            if (closing()) {
                                data.putString(str);
                            } else {
                                str = "HELLO";
                                data.putString(str);
                            }
                        } catch (BufferOverflowException e) {
                            close(true);
                        }
                        Log.debug("[" + this + "] sending data "+data+" from java:'" + str + "' !");
                    }
                };
            }
        });
        c.start();
        c.join();

        Aio4c.end();
    }
}
