/**
 * This file is part of Aio4c <http://aio4c.so>.
 *
 * Aio4c <http://aio4c.so> is free software: you
 * can  redistribute  it  and/or modify it under
 * the  terms  of the GNU General Public License
 * as published by the Free Software Foundation,
 * version 3 of the License.
 *
 * This  program is distributed in the hope that
 * it  will be useful, but WITHOUT ANY WARRANTY;
 * without   even   the   implied   warranty  of
 * MERCHANTABILITY  or  FITNESS FOR A PARTICULAR
 * PURPOSE.
 *
 * See  the  GNU General Public License for more
 * details.  You  should have received a copy of
 * the  GNU  General  Public  License along with
 * Aio4c    <http://aio4c.so>.   If   not,   see
 * <http://www.gnu.org/licenses/>.
 */
import com.aio4c.Aio4c;
import com.aio4c.Connection;
import com.aio4c.ConnectionFactory;
import com.aio4c.Log;
import com.aio4c.ServerConfig;
import com.aio4c.buffer.Buffer;
import com.aio4c.buffer.BufferOverflowException;

public class Server {
    public static void main(String[] args) {
        Aio4c.init(args);
        Thread t = new Thread() {
            @Override
            public void run() {
                final com.aio4c.Server server = new com.aio4c.Server(new ServerConfig(), new ConnectionFactory() {
                    @Override
                    public Connection create () {
                        return new Connection () {
                            private boolean first = true;
                            @Override
                            public void onInit() {
                                Log.debug("["+this+"] initialized");
                            }
                            @Override
                            public void onConnect() {
                                Log.debug("["+this+"] connected");
                                enableWriteInterest();
                            }
                            @Override
                            public void onClose() {
                                Log.debug("["+this+"] closed");
                            }
                            @Override
                            public void onRead(Buffer data) {
                                String s = data.getString();
                                Log.debug("["+this+"] received '"+s+"', buffer "+data);
                                if (first) {
                                    enableWriteInterest();
                                    first = false;
                                }
                            }
                            @Override
                            public void onWrite(Buffer data) {
                                try {
                                    data.putString("BONJOUR");
                                    data.putString("QUIT");
                                    Log.debug("["+this+"] sending "+data);
                                } catch (BufferOverflowException e) {
                                    e.printStackTrace();
                                    close(true);
                                }
                            }
                        };
                    }
                });
                Runtime.getRuntime().addShutdownHook(new Thread() {
                    public void run() {
                        server.stop();
                        Aio4c.end();
                    }
                });
                server.join();
            }
        };
        t.setDaemon(false);
        t.start();
    }
}
