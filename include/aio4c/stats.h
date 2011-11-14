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
#ifndef __AIO4C_STATS_H__
#define __AIO4C_STATS_H__

#include <aio4c/types.h>

#include <stdio.h>
#include <sys/time.h>

#ifndef AIO4C_ENABLE_STATS
#define AIO4C_ENABLE_STATS 0
#endif /* AIO4C_ENABLE_STATS */

#if AIO4C_ENABLE_STATS

#define ProbeTimeStart(type) {                 \
    struct timeval _start, _stop;              \
    gettimeofday(&_start, NULL);

#define ProbeTimeEnd(type)                     \
    gettimeofday(&_stop, NULL);                \
    _ProbeTime(type,&_start,&_stop);           \
}

#define ProbeTime(type,start,stop) \
    _ProbeTime(type,start,stop)

#define ProbeSize(type,value) \
    _ProbeSize(type,value)

#else /* AIO4C_ENABLE_STATS */
#define ProbeTimeStart(type)
#define ProbeTimeEnd(type)
#define ProbeTime(type,start,stop)
#define ProbeSize(type,value)
#endif /* AIO4C_ENABLE_STATS */

#define pstats(fmt, ...) \
    fprintf(stderr, fmt, __VA_ARGS__)

#if AIO4C_ENABLE_STATS

typedef enum e_ProbeTimeType {
    AIO4C_TIME_PROBE_MEMORY_ALLOCATION,
    AIO4C_TIME_PROBE_BUFFER_ALLOCATION,
    AIO4C_TIME_PROBE_NETWORK_READ,
    AIO4C_TIME_PROBE_NETWORK_WRITE,
    AIO4C_TIME_PROBE_SELECT_OVERHEAD,
    AIO4C_TIME_PROBE_DATA_PROCESS,
    AIO4C_TIME_PROBE_BLOCK,
    AIO4C_TIME_PROBE_IDLE,
    AIO4C_TIME_PROBE_LATENCY,
    AIO4C_TIME_PROBE_JNI_OVERHEAD,
    AIO4C_TIME_MAX_PROBE_TYPE
} ProbeTimeType;

typedef enum e_ProbeSizeType {
    AIO4C_PROBE_MEMORY_ALLOCATED_SIZE,
    AIO4C_PROBE_BUFFER_ALLOCATED_SIZE,
    AIO4C_PROBE_NETWORK_READ_SIZE,
    AIO4C_PROBE_NETWORK_WRITE_SIZE,
    AIO4C_PROBE_PROCESSED_DATA_SIZE,
    AIO4C_PROBE_CONNECTION_COUNT,
    AIO4C_PROBE_MEMORY_ALLOCATE_COUNT,
    AIO4C_PROBE_MEMORY_FREE_COUNT,
    AIO4C_PROBE_LATENCY_COUNT,
    AIO4C_PROBE_MAX_SIZE_TYPE
} ProbeSizeType;

extern int AIO4C_STATS_INTERVAL;

extern AIO4C_API bool AIO4C_STATS_ENABLE_PERIODIC_OUTPUT;

extern char* AIO4C_STATS_OUTPUT_FILE;

extern AIO4C_API void StatsInit(void);

extern AIO4C_API void _ProbeTime(ProbeTimeType type, struct timeval* start, struct timeval* stop);

extern AIO4C_API void _ProbeSize(ProbeSizeType type, int size);

extern AIO4C_API void StatsEnd(void);

#endif /* AIO4C_ENABLE_STATS */

#endif
