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
#ifndef __AIO4C_STATS_H__
#define __AIO4C_STATS_H__

#include <stdio.h>
#include <sys/time.h>

#ifndef AIO4C_ENABLE_STATS
#define AIO4C_ENABLE_STATS 0
#endif

#if AIO4C_ENABLE_STATS == 1
#define ProbeTimeStart(type) {     \
    struct timeval _start, _stop;  \
    gettimeofday(&_start, NULL);
#define ProbeTimeEnd(type)         \
    gettimeofday(&_stop, NULL);    \
    _ProbeTime(type,&_start,&_stop); \
}
#define ProbeSize(type,value) \
    _ProbeSize(type,(long)value)
#else
#define ProbeTimeStart(type)
#define ProbeTimeEnd(type)
#define ProbeSize(type,value)
#endif

#define pstats(fmt, ...) \
    fprintf(stderr, fmt, __VA_ARGS__)

typedef enum e_ProbeTimeType {
    TIME_PROBE_MEMORY_ALLOCATION,
    TIME_PROBE_NETWORK_READ,
    TIME_PROBE_NETWORK_WRITE,
    TIME_PROBE_DATA_PROCESS,
    TIME_PROBE_BLOCK,
    TIME_PROBE_IDLE,
    TIME_MAX_PROBE_TYPE
} ProbeTimeType;

typedef enum e_ProbeSizeType {
    PROBE_MEMORY_ALLOCATED_SIZE,
    PROBE_NETWORK_READ_SIZE,
    PROBE_NETWORK_WRITE_SIZE,
    PROBE_PROCESSED_DATA_SIZE,
    PROBE_CONNECTIONS_SIZE,
    PROBE_MAX_SIZE_TYPE
} ProbeSizeType;

#if AIO4C_ENABLE_STATS == 1
extern void _InitProbes(void) __attribute__((constructor));
#endif

extern void _ProbeTime(ProbeTimeType type, struct timeval* start, struct timeval* stop);

extern void _ProbeSize(ProbeSizeType type, long size);

#if AIO4C_ENABLE_STATS == 1
extern void _StatsEnd(void) __attribute__((destructor));
#endif

#endif
