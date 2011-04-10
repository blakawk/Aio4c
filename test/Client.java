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
import com.aio4c.Aio4c;
import com.aio4c.Buffer;
import com.aio4c.ClientConfig;
import com.aio4c.Connection;
import com.aio4c.ConnectionFactory;
import com.aio4c.Log;

public class Client {
    public static void main(String[] args) {
        Aio4c.init(Log.Level.DEBUG, "jclient.log");

        com.aio4c.Client c = new com.aio4c.Client(new ClientConfig(), new ConnectionFactory () {
            @Override
            public Connection create() {
                return new Connection () {
                    private boolean toClose = false;
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
                                toClose = true;
                            }
                        }
                        enableWriteInterest();
                    }
                    @Override
                    public void onWrite(Buffer data) {
                        if (closing()) {
                            data.putString("GOODBYE");
                        } else {
                            data.putString("HELLO");
                            if (toClose) {
                                close(false);
                            }
                        }
                        Log.debug("[" + this + "] sending data "+data+" from java !");
                    }
                };
            }
        });
        c.join();

        Aio4c.end();
    }
}
