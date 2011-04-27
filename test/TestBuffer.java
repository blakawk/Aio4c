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
import com.aio4c.buffer.Buffer;
import com.aio4c.buffer.BufferOverflowException;
import com.aio4c.buffer.BufferUnderflowException;

public class TestBuffer {
    private static final int BUFSZ = 10;

    public static void main(String[] args) {
        Aio4c.init(args);
        try {
            try {
                Buffer b = Buffer.allocate(Integer.MAX_VALUE);
                System.out.println("KO (1)");
            } catch (OutOfMemoryError oom) {
                oom.printStackTrace();
                Buffer b = Buffer.allocate(BUFSZ);
                try {
                    for (int i = 0; i <= BUFSZ; i++) {
                        b.get();
                    }
                    System.out.println("KO (2)");
                } catch (BufferUnderflowException bu) {
                    bu.printStackTrace();
                    try {
                        b.position(BUFSZ + 1);
                        System.out.println("KO (3)");
                    } catch (IllegalArgumentException ia) {
                        ia.printStackTrace();
                        b.reset();
                        try {
                            for (int i = 0; i <= BUFSZ; i++) {
                                b.put((byte)1);
                            }
                            System.out.println("KO (4)");
                        } catch (BufferOverflowException bo) {
                            bo.printStackTrace();
                            b.flip();
                            if (b.position() == 0 && b.limit() == BUFSZ) {
                                System.out.println("OK");
                            } else {
                                System.out.println("KO (5)");
                            }
                        }
                    }
                }
            }
        } catch (Throwable t) {
            t.printStackTrace();
            System.out.println("KO");
        }
        Aio4c.end();
    }
}
