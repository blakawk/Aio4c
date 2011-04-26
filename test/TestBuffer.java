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
import com.aio4c.buffer.Buffer;
import com.aio4c.buffer.BufferUnderflowException;

public class TestBuffer {
    public static void main(String[] args) {
        Aio4c.init(args);
        Buffer b = Buffer.allocate(5);
        try {
            b.get();
            b.get();
            b.get();
            b.get();
            b.get();
            b.get();
        } catch (BufferUnderflowException e) {
            e.printStackTrace();
        }
        Aio4c.end();
    }
}
