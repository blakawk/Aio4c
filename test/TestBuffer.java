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
import com.aio4c.buffer.Buffer;
import com.aio4c.buffer.BufferOverflowException;
import com.aio4c.buffer.BufferUnderflowException;

public class TestBuffer {
    private static final int BUFSZ = 10;

    public static void main(String[] args) {
        Aio4c.init(args, null);
        try {
            try {
                Buffer b = Buffer.allocate(Integer.MAX_VALUE);
                System.exit(1);
            } catch (OutOfMemoryError oom) {
                Buffer b = Buffer.allocate(BUFSZ);
                try {
                    for (int i = 0; i <= BUFSZ; i++) {
                        b.get();
                    }
                    System.exit(2);
                } catch (BufferUnderflowException bu) {
                    try {
                        b.position(BUFSZ + 1);
                        System.exit(3);
                    } catch (IllegalArgumentException ia) {
                        b.reset();
                        try {
                            for (int i = 0; i <= BUFSZ; i++) {
                                b.put((byte)1);
                            }
                            System.exit(4);
                        } catch (BufferOverflowException bo) {
                            b.flip();
                            if (b.position() == 0 && b.limit() == BUFSZ) {
                                b.putString("€");
                                b.flip();
                                if (b.getString().equals("€")) {
                                    ;
                                } else {
                                    System.exit(6);
                                }
                            } else {
                                System.exit(5);
                            }
                        }
                    }
                }
            }
        } catch (Throwable t) {
            System.exit(-1);
        }
        Aio4c.end();
        System.exit(0);
    }
}
