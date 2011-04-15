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

#include <limits.h>
#include <stdlib.h>
#include <strings.h>
#include <unistd.h>

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

void Aio4cUsage(void) {
    fprintf(stderr, "Aio4c options:\n");
    fprintf(stderr, "\t-Ah: displays this help message\n");
    fprintf(stderr, "\t-Ll loglevel: loglevel (as integer or string) between the following:\n");
    fprintf(stderr, "\t\tFATAL(0): displays only fatal errors\n");
    fprintf(stderr, "\t\tERROR(1): displays non fatal errors\n");
    fprintf(stderr, "\t\tWARN(2) : displays warnings\n");
    fprintf(stderr, "\t\tINFO(3) : displays informations\n");
    fprintf(stderr, "\t\tDEBUG(4): displays debugging informations\n");
    fprintf(stderr, "\t\t*Note*: each log level displays it's level message plus messages of lower level\n");
    fprintf(stderr, "\t-Lo logfile : where to output the logs (default: stderr)\n");
#ifdef AIO4C_ENABLE_STATS
    fprintf(stderr, "\t-So statfile: where to output stats (default: stats-[PID].csv)\n");
    fprintf(stderr, "\t-Si interval: defines the statistics sample interval (default: 1s)\n");
    fprintf(stderr, "\t-Sd         : if set, disable periodic statistics output to stderr (default: enabled)\n");
#endif /* AIO4C_ENABLE_STATS */
}
static void _ParseArguments(int argc, char* argv[]) {
    int optind = 0;
    char* logfile = NULL;
    LogLevel level = AIO4C_LOG_LEVEL_FATAL;
    long int value = 0;
    char* levelStr = NULL;
    char* endptr = NULL;

    for (optind = 1; optind < argc; optind++) {
        switch(argv[optind][0]) {
            case '-':
                switch (argv[optind][1]) {
                    case 'L':
                        switch (argv[optind][2]) {
                            case 'o':
                                if (optind + 1 < argc) {
                                    logfile = argv[optind + 1];
                                    optind++;
                                }
                                break;
                            case 'l':
                                if (optind + 1 < argc) {
                                    levelStr = argv[optind + 1];
                                    if (strcasecmp("fatal", levelStr) == 0) {
                                        level = AIO4C_LOG_LEVEL_FATAL;
                                    } else if (strcasecmp("error", levelStr) == 0) {
                                        level = AIO4C_LOG_LEVEL_ERROR;
                                    } else if (strcasecmp("warn", levelStr) == 0) {
                                        level = AIO4C_LOG_LEVEL_WARN;
                                    } else if (strcasecmp("info", levelStr) == 0) {
                                        level = AIO4C_LOG_LEVEL_INFO;
                                    } else if (strcasecmp("debug", levelStr) == 0) {
                                        level = AIO4C_LOG_LEVEL_DEBUG;
                                    } else {
                                        value = 0;
                                        value = strtol(argv[optind + 1], &endptr, 10);
                                        if (value < AIO4C_LOG_LEVEL_MAX && value >= 0) {
                                            level = (LogLevel)value;
                                        }
                                    }
                                    optind++;
                                }
                                break;
                            default:
                                break;
                        }
                        break;
#ifdef AIO4C_ENABLE_STATS
                    case 'S':
                        switch (argv[optind][2]) {
                            case 'o':
                                if (optind + 1 < argc) {
                                    AIO4C_STATS_OUTPUT_FILE = argv[optind + 1];
                                    optind++;
                                }
                                break;
                            case 'i':
                                if (optind + 1 < argc) {
                                    value = 0;
                                    value = strtol(argv[optind + 1], &endptr, 10);
                                    if (value >= 0 && value < INT_MAX) {
                                        AIO4C_STATS_INTERVAL = (int)value;
                                    }
                                    optind++;
                                }
                                break;
                            case 'd':
                                AIO4C_STATS_ENABLE_PERIODIC_OUTPUT = false;
                                break;
                            default:
                                break;
                        }
                        break;
#endif /* AIO4C_ENABLE_STATS */
                    case 'A':
                        switch (argv[optind][2]) {
                            case 'h':
                                Aio4cUsage();
                                Aio4cEnd();
                                exit(EXIT_SUCCESS);
                                break;
                            default:
                                break;
                        }
                        break;
                    default:
                        break;
                }
                break;
            default:
                break;
        }
    }

    LogInit(level, logfile);
}

void Aio4cInit(int argc, char* argv[]) {
    _ParseArguments(argc, argv);
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
