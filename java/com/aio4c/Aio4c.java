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
package com.aio4c;

import com.aio4c.log.Logger;

/**
 * Class allowing initialization of aio4c library.
 */
public class Aio4c {
    static {
        /**
         * Try to load the library.
         */
        System.loadLibrary("aio4c");
    }
    /**
     * Displays the parameters and usage of the library
     */
    public static native void usage();
    /**
     * Initializes aio4c.
     *
     * Needs to be called once before using the library.
     *
     * @param args
     *   The arguments passed on the JVM command line
     * @param logger
     *   A {@link Logger} implementation used by the library to log messages.
     *   This parameter can be <code>null</code>, then the library will uses
     *   a default logging implementation that can be parameterized according
     *   to provided command-line arguments.
     */
    public static native void init(String[] args, Logger logger);
    /**
     * Frees all resources used by aio4c.
     *
     * Needs to be called once the library is no more used by the program.
     */
    public static native void end();
}
