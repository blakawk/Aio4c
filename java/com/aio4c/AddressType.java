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
package com.aio4c;

/**
 * Enum that represents the kinds of addresses supported by the library.
 *
 * @author blakawk
 * @see "aio4c/address.h"
 */
public enum AddressType {
    /**
     * Address is an IPv4.
     */
    IPV4(0),
    /**
     * Address is an IPv6
     */
    IPV6(1);
    /**
     * The enum's value.
     */
    public int value;
    /**
     * The AddressType constructor.
     *
     * @param int
     *   The enum's value.
     */
    private AddressType(int value) {
        this.value = value;
    }
}
