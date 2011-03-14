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
import com.aio4c.LogLevel;

public class Client {
    public static void main(String[] args) {
        Aio4c.init(LogLevel.DEBUG, "jclient.log");

        com.aio4c.Client c = new com.aio4c.Client(new ClientConfig(), new ConnectionFactory () {
            @Override
            public Connection create() {
                return new Connection () {
                    @Override
                    public void onInit() {
                        System.out.println("[" + this + "] init from java !");
                    }
                    @Override
                    public void onClose() {
                        System.out.println("[" + this + "] close from java !");
                    }
                    @Override
                    public void onConnect() {
                        System.out.println("[" + this + "] connect from java !");
                    }
                    @Override
                    public void onRead(Buffer data) {
                        String str = data.getString();
                        System.out.println("[" + this + "] received data from java: '" + str + "' !");
                        if (str.equals("QUIT")) {
                            close();
                        } else {
                            enableWriteInterest();
                        }
                    }
                    @Override
                    public void onWrite(Buffer data) {
                        System.out.println("[" + this + "] sending data from java !");
                        data.putString("HELLO\n");
                    }
                };
            }
        });
        c.join();

        Aio4c.end();
    }
}
