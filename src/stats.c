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
#include <aio4c/stats.h>

#if AIO4C_ENABLE_STATS

#include <aio4c/log.h>
#include <aio4c/thread.h>
#include <aio4c/types.h>

#ifdef AIO4C_WIN32

#include <winbase.h>

#else /* AIO4C_WIN32 */

#include <pthread.h>
#include <sys/types.h>
#include <unistd.h>

#endif /* AIO4C_WIN32 */

#include <errno.h>
#include <stdio.h>
#include <string.h>

char* AIO4C_STATS_OUTPUT_FILE = NULL;
bool AIO4C_STATS_ENABLE_PERIODIC_OUTPUT = false;
int AIO4C_STATS_INTERVAL = 0;

static Thread*          _statsThread = NULL;
static FILE*            _statsFile = NULL;
static double           _timeProbes[AIO4C_TIME_MAX_PROBE_TYPE];
static double           _sizeProbes[AIO4C_PROBE_MAX_SIZE_TYPE];
#ifndef AIO4C_WIN32
static pthread_mutex_t  _timeProbesLock;
static pthread_mutex_t  _sizeProbesLock;
#else /* AIO4C_WIN32 */
static CRITICAL_SECTION _timeProbesLock;
static CRITICAL_SECTION _sizeProbesLock;
#endif /* AIO4C_WIN32 */

static bool _statsInit(void* dummy __attribute__((unused))) {
    dummy = NULL;
#ifndef AIO4C_WIN32
    pid_t pid = getpid();
#else /* AIO4C_WIN32 */
    DWORD pid = GetCurrentProcessId();
#endif /* AIO4C_WIN32 */

    char filename[128];

    if (AIO4C_STATS_OUTPUT_FILE == NULL) {
        memset(filename, 0, 128);
        snprintf(filename, 128, "stats-%ld.csv", (long)pid);
        AIO4C_STATS_OUTPUT_FILE = (char*)filename;
    }

    _statsFile = fopen(AIO4C_STATS_OUTPUT_FILE, "w");

    if (_statsFile != NULL) {
        fprintf(_statsFile, "TIME (s);ALLOCATED MEMORY (kb);READ DATA (kb/s);WRITTEN DATA (kb/s);PROCESSED DATA (kb/s);CONNECTIONS;IDLE TIME (ms);LATENCY (ms);SELECT OVERHEAD (ms)\n");
    } else {
        Log(AIO4C_LOG_LEVEL_WARN, "cannot open stat file %s for writing: %s\n", AIO4C_STATS_OUTPUT_FILE, strerror(errno));
        return false;
    }

    return true;
}

void _PrintStats(void);

void _WriteStats(void);

static bool _statsRun(void* dummy __attribute__((unused))) {
    dummy = NULL;

    if (AIO4C_STATS_ENABLE_PERIODIC_OUTPUT) {
        _PrintStats();
    }

    if (_statsFile != NULL) {
        _WriteStats();
    }

#ifdef AIO4C_WIN32
    Sleep(AIO4C_STATS_INTERVAL * 1000);
#else /* AIO4C_WIN32 */
    sleep(AIO4C_STATS_INTERVAL);
#endif /* AIO4C_WIN32 */

    return true;
}

static void _statsExit(void* dummy __attribute__((unused))) {
    dummy = NULL;
    if (_statsFile != NULL) {
        fclose(_statsFile);
    }
}

void StatsInit(void) {
    memset(_timeProbes, 0, AIO4C_TIME_MAX_PROBE_TYPE * sizeof(double));
    memset(_sizeProbes, 0, AIO4C_PROBE_MAX_SIZE_TYPE * sizeof(double));

#ifndef AIO4C_WIN32
    pthread_mutex_init(&_timeProbesLock, NULL);
    pthread_mutex_init(&_sizeProbesLock, NULL);
#else /* AIO4C_WIN32 */
    InitializeCriticalSection(&_timeProbesLock);
    InitializeCriticalSection(&_sizeProbesLock);
#endif /* AIO4C_WIN32 */

    _statsThread = NULL;
    if (AIO4C_STATS_INTERVAL) {
        _statsThread = NewThread("stats",
                aio4c_thread_init(_statsInit),
                aio4c_thread_run(_statsRun),
                aio4c_thread_exit(_statsExit),
                aio4c_thread_arg(NULL));

        if (_statsThread != NULL && !ThreadStart(_statsThread)) {
            FreeThread(&_statsThread);
        }
    }
}

void _ProbeTime(ProbeTimeType type, struct timeval* start, struct timeval* stop) {
#ifndef AIO4C_WIN32
    pthread_mutex_lock(&_timeProbesLock);
#else /* AIO4C_WIN32 */
    EnterCriticalSection(&_timeProbesLock);
#endif /* AIO4C_WIN32 */

    _timeProbes[type] += (double)(stop->tv_sec - start->tv_sec) * 1000000.0 + (double)(stop->tv_usec - start->tv_usec);

#ifndef AIO4C_WIN32
    pthread_mutex_unlock(&_timeProbesLock);
#else /* AIO4C_WIN32 */
    LeaveCriticalSection(&_timeProbesLock);
#endif /* AIO4C_WIN32 */
}

void _ProbeSize(ProbeSizeType type, int size) {
#ifndef AIO4C_WIN32
    pthread_mutex_lock(&_sizeProbesLock);
#else /* AIO4C_WIN32 */
    EnterCriticalSection(&_sizeProbesLock);
#endif /* AIO4C_WIN32 */

    _sizeProbes[type] += (double)size;

#ifndef AIO4C_WIN32
    pthread_mutex_unlock(&_sizeProbesLock);
#else /* AIO4C_WIN32 */
    LeaveCriticalSection(&_sizeProbesLock);
#endif /* AIO4C_WIN32 */
}

static double _elapsedTime(void) {
    static struct timeval _start;
    static bool _initialized = false;
    struct timeval _stop;
    double result = 0.0;

    if (!_initialized) {
        gettimeofday(&_start, NULL);
        _initialized = true;
    }

    gettimeofday(&_stop, NULL);

    result = (double)(_stop.tv_sec - _start.tv_sec) * 1000000.0 + (double)(_stop.tv_usec - _start.tv_usec);

    if (GetNumThreads() > 0) {
        result *= GetNumThreads();
    }

    return result;
}

static void _ConvertSize(double size, double* result, char** unit) {
    if (size < 1024.0) {
        *result = size;
        *unit = "b";
        return;
    }

    size /= 1024.0;

    if (size < 1024.0) {
        *result = size;
        *unit = "kb";
        return;
    }

    size /= 1024.0;

    if (size < 1024.0) {
        *result = size;
        *unit = "mb";
        return;
    }

    size /= 1024.0;

    if (size < 1024.0) {
        *result = size;
        *unit = "gb";
        return;
    }

    size /= 1024.0;

    *result = size;
    *unit = "tb";
}

static void _ptimes(char* label, ProbeTimeType tType) {
#ifndef AIO4C_WIN32
    pthread_mutex_lock(&_timeProbesLock);
#else /* AIO4C_WIN32 */
    EnterCriticalSection(&_timeProbesLock);
#endif /* AIO4C_WIN32 */

    pstats("=== %s: %.3f s [%.3f%%]\n", label, _timeProbes[tType] / 1000000.0,
            _timeProbes[tType] * 100.0 / _elapsedTime());

#ifndef AIO4C_WIN32
    pthread_mutex_unlock(&_timeProbesLock);
#else /* AIO4C_WIN32 */
    LeaveCriticalSection(&_timeProbesLock);
#endif /* AIO4C_WIN32 */
}

static void _pstats(char* label, ProbeTimeType tType, ProbeSizeType sType) {
    double size = 0.0;
    char* unit = NULL;

#ifndef AIO4C_WIN32
    pthread_mutex_lock(&_sizeProbesLock);
#else /* AIO4C_WIN32 */
    EnterCriticalSection(&_sizeProbesLock);
#endif /* AIO4C_WIN32 */

    _ConvertSize(_sizeProbes[sType], &size, &unit);

#ifndef AIO4C_WIN32
    pthread_mutex_unlock(&_sizeProbesLock);
    pthread_mutex_lock(&_timeProbesLock);
#else /* AIO4C_WIN32 */
    LeaveCriticalSection(&_sizeProbesLock);
    EnterCriticalSection(&_timeProbesLock);
#endif /* AIO4C_WIN32 */

    pstats("=== %s: %.3f %s in %.3f s [%.3f%%]\n", label, size, unit,
            _timeProbes[tType] / 1000000.0,
            _timeProbes[tType] * 100.0 / _elapsedTime());

#ifndef AIO4C_WIN32
    pthread_mutex_unlock(&_timeProbesLock);
#else /* AIO4C_WIN32 */
    LeaveCriticalSection(&_timeProbesLock);
#endif /* AIO4C_WIN32 */
}

void _PrintStats(void) {
    static double lastAlloc = 0.0, lastFree = 0.0;
    long _alloc = 0L, _free = 0L;

#ifndef AIO4C_WIN32
    pthread_mutex_lock(&_sizeProbesLock);
#else /* AIO4C_WIN32 */
    EnterCriticalSection(&_sizeProbesLock);
#endif /* AIO4C_WIN32 */

    _alloc = (long)_sizeProbes[AIO4C_PROBE_MEMORY_ALLOCATE_COUNT] - lastAlloc;
    lastAlloc = _sizeProbes[AIO4C_PROBE_MEMORY_ALLOCATE_COUNT];
    _free = (long)_sizeProbes[AIO4C_PROBE_MEMORY_FREE_COUNT] - lastFree;
    lastFree = _sizeProbes[AIO4C_PROBE_MEMORY_FREE_COUNT];

#ifndef AIO4C_WIN32
    pthread_mutex_unlock(&_sizeProbesLock);
#else /* AIO4C_WIN32 */
    LeaveCriticalSection(&_sizeProbesLock);
#endif /* AIO4C_WIN32 */

     pstats("================= STATISTICS ===================%c", '\n');
    _pstats("ALLOCATED MEMORY ", AIO4C_TIME_PROBE_MEMORY_ALLOCATION, AIO4C_PROBE_MEMORY_ALLOCATED_SIZE);
    _pstats("ALLOCATED BUFFERS", AIO4C_TIME_PROBE_BUFFER_ALLOCATION, AIO4C_PROBE_BUFFER_ALLOCATED_SIZE);
    pstats("=== MEMORY ALLOCATION: %ld allocations, %ld frees\n", _alloc, _free);
    _pstats("NETWORK READ     ", AIO4C_TIME_PROBE_NETWORK_READ, AIO4C_PROBE_NETWORK_READ_SIZE);
    _pstats("NETWORK WRITE    ", AIO4C_TIME_PROBE_NETWORK_WRITE, AIO4C_PROBE_NETWORK_WRITE_SIZE);
    _pstats("PROCESSED DATA   ", AIO4C_TIME_PROBE_DATA_PROCESS, AIO4C_PROBE_PROCESSED_DATA_SIZE);
    _pstats("LATENCY          ", AIO4C_TIME_PROBE_LATENCY, AIO4C_PROBE_LATENCY_COUNT);
    _ptimes("IDLE TIME        ", AIO4C_TIME_PROBE_IDLE);
    _ptimes("BLOCKED TIME     ", AIO4C_TIME_PROBE_BLOCK);
    _ptimes("JNI OVERHEAD     ", AIO4C_TIME_PROBE_JNI_OVERHEAD);
     pstats("=== RUNNING THREADS  : %d\n", GetNumThreads());
     pstats("=================    END     ===================%c", '\n');
}

#define floatWithComma(f) (int)(f), (int)(((f) - (double)((int)(f))) * 1000000.0)

void _WriteStats(void) {
    static struct timeval _start;
    static bool _initialized = false;
    struct timeval time;
    static double lastRead = 0.0, lastWrite = 0.0, lastProcess = 0.0, lastIdle = 0.0, lastLatency = 0.0, lastLatencyCount = 0.0, lastSelectOverhead = 0.0;
    double read = 0.0, write = 0.0, process = 0.0, idle = 0.0, allocated = 0.0, connections = 0.0, latency = 0.0, latencyCount = 0.0, selectOverhead = 0.0;

    if (_statsFile == NULL) {
        return;
    }

    if (!_initialized) {
        _initialized = true;
        gettimeofday(&_start, NULL);
    }

    gettimeofday(&time, NULL);

#ifndef AIO4C_WIN32
    pthread_mutex_lock(&_timeProbesLock);
    pthread_mutex_lock(&_sizeProbesLock);
#else /* AIO4C_WIN32 */
    EnterCriticalSection(&_timeProbesLock);
    EnterCriticalSection(&_sizeProbesLock);
#endif /* AIO4C_WIN32 */

    read =  _sizeProbes[AIO4C_PROBE_NETWORK_READ_SIZE] - lastRead;
    lastRead = _sizeProbes[AIO4C_PROBE_NETWORK_READ_SIZE];
    write = _sizeProbes[AIO4C_PROBE_NETWORK_WRITE_SIZE] - lastWrite;
    lastWrite = _sizeProbes[AIO4C_PROBE_NETWORK_WRITE_SIZE];
    process = _sizeProbes[AIO4C_PROBE_PROCESSED_DATA_SIZE] - lastProcess;
    lastProcess = _sizeProbes[AIO4C_PROBE_PROCESSED_DATA_SIZE];
    latencyCount = _sizeProbes[AIO4C_PROBE_LATENCY_COUNT] - lastLatencyCount;
    lastLatencyCount = _sizeProbes[AIO4C_PROBE_LATENCY_COUNT];
    idle = _timeProbes[AIO4C_TIME_PROBE_IDLE] - lastIdle;
    lastIdle = _timeProbes[AIO4C_TIME_PROBE_IDLE];
    allocated = _sizeProbes[AIO4C_PROBE_MEMORY_ALLOCATED_SIZE];
    connections = _sizeProbes[AIO4C_PROBE_CONNECTION_COUNT];
    latency = _timeProbes[AIO4C_TIME_PROBE_LATENCY] - lastLatency;
    lastLatency = _timeProbes[AIO4C_TIME_PROBE_LATENCY];
    selectOverhead = _timeProbes[AIO4C_TIME_PROBE_SELECT_OVERHEAD] - lastSelectOverhead;
    lastSelectOverhead = _timeProbes[AIO4C_TIME_PROBE_SELECT_OVERHEAD];

#ifndef AIO4C_WIN32
    pthread_mutex_unlock(&_sizeProbesLock);
    pthread_mutex_unlock(&_timeProbesLock);
#else /* AIO4C_WIN32 */
    LeaveCriticalSection(&_sizeProbesLock);
    LeaveCriticalSection(&_timeProbesLock);
#endif /* AIO4C_WIN32 */

    fprintf(_statsFile, "%u;%d,%d;%d,%d;%d,%d;%d,%d;%d;%d,%d;%d,%d;%d,%d\n", (unsigned int)(time.tv_sec - _start.tv_sec),
            floatWithComma(allocated / 1024.0),
            floatWithComma(read / 1024.0), floatWithComma(write / 1024.0), floatWithComma(process / 1024.0), (int)connections,
            floatWithComma(idle / 1000.0), floatWithComma(latencyCount>0?(latency / 1000.0 / latencyCount):0.0),
            floatWithComma(selectOverhead>0.0?(selectOverhead / 1000.0 / write):0.0));
}

void StatsEnd(void) {
    if (_statsThread != NULL) {
        _statsThread->running = false;
        ThreadJoin(_statsThread);
    }
    if (AIO4C_STATS_ENABLE_PERIODIC_OUTPUT) {
        _PrintStats();
    }
}

#endif /* AIO4C_ENABLE_STATS */
