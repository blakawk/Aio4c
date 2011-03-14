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
#include <aio4c.h>

#include <aio4c/log.h>
#include <aio4c/stats.h>
#include <aio4c/thread.h>

#ifdef AIO4C_WIN32
#include <winbase.h>
#include <winsock2.h>

static void _RetrieveWinVer(void) __attribute__((constructor));
static void _InitWinSock(void) __attribute__((constructor));
static void _CleanUpWinSock(void) __attribute__((destructor));

static void _InitWinSock(void) {
    WORD v = MAKEWORD(2,2);
    WSADATA d;
    int result = WSAStartup(v, &d);
    if (result != 0) {
        fprintf(stderr, "Cannot initialize WinSock2: 0x%08d\n", result);
        exit(EXIT_FAILURE);
    }

    dthread("WinSock2 started up (version %d.%d)\n", LOBYTE(d.wVersion), HIBYTE(d.wVersion));
}

static void _CleanUpWinSock(void) {
    if (WSACleanup() == SOCKET_ERROR) {
        dthread("WinSock2 cannot be cleaned up: 0x%08d\n", WSAGetLastError());
    } else {
        if (AIO4C_DEBUG_THREADS) {
            fprintf(stderr, "WinSock2 cleaned up\n");
        }
    }
}

static void _RetrieveWinVer(void) {
    DWORD version = 0;
    WORD  majorMinor = 0;

    version = GetVersion();
    majorMinor = MAKEWORD(HIBYTE(LOWORD(version)), LOBYTE(LOWORD(version)));
 
    if (majorMinor < WINVER) {
        fprintf(stderr, "Windows version 0x%04x is too old (required: at least 0x%04x)\n",
                majorMinor, WINVER);
        exit(EXIT_FAILURE);
    }

    dthread("Windows version detected: 0x%04x (required: 0x%04x)\n", majorMinor, WINVER);
}
#endif /* AIO4C_WIN32 */

void Aio4cInit(LogLevel level, char* log) {
    LogInit(level, log);
#if AIO4C_ENABLE_STATS
    StatsInit();
#endif /* AIO4C_ENABLE_STATS */
}

void Aio4cEnd(void) {
    LogEnd();
#if AIO4C_ENABLE_STATS
    StatsEnd();
#endif /* AIO4C_ENABLE_STATS */
}
