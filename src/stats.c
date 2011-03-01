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
#include <aio4c/stats.h>

#include <aio4c/thread.h>

#include <string.h>
#include <unistd.h>

static double       _timeProbes[TIME_MAX_PROBE_TYPE];
static aio4c_lock_t _timeProbesLock;
static double       _sizeProbes[PROBE_MAX_SIZE_TYPE];
static aio4c_lock_t _sizeProbesLock;
static Thread*      _statsThread = NULL;
static FILE*        _statsFile = NULL;

static void _statsInit(void* dummy) {
    dummy = NULL;
    _statsFile = fopen("stats.csv", "w");
    if (_statsFile != NULL) {
        fprintf(_statsFile, "TIME (s);ALLOCATED MEMORY (kb);READ DATA (kb/s);WRITTEN DATA (kb/s);PROCESSED DATA (kb/s);CONNECTIONS;IDLE TIME (ms);LATENCY (ms)\r\n");
    }
}

void _PrintStats(void);

void _WriteStats(void);

static aio4c_bool_t _statsRun(void* dummy) {
    dummy = NULL;

    _PrintStats();
    if (_statsFile != NULL) {
        _WriteStats();
    }

    sleep(1);

    return true;
}

static void _statsExit(void* dummy) {
    dummy = NULL;
    fclose(_statsFile);
}

void _InitProbes(void) {
    memset(_timeProbes, 0, TIME_MAX_PROBE_TYPE * sizeof(double));
    memset(_sizeProbes, 0, PROBE_MAX_SIZE_TYPE * sizeof(double));
    aio4c_mutex_init(&_timeProbesLock, NULL);
    aio4c_mutex_init(&_sizeProbesLock, NULL);
    _statsThread = NewThread("stats", aio4c_thread_handler(_statsInit), aio4c_thread_run(_statsRun), aio4c_thread_handler(_statsExit), aio4c_thread_arg(NULL));
}

void _ProbeTime(ProbeTimeType type, struct timeval* start, struct timeval* stop) {
    aio4c_mutex_lock(&_timeProbesLock);
    _timeProbes[type] += (double)(stop->tv_sec - start->tv_sec) * 1000000.0 + (double)(stop->tv_usec - start->tv_usec);
    aio4c_mutex_unlock(&_timeProbesLock);
}

void _ProbeSize(ProbeSizeType type, int size) {
    aio4c_mutex_lock(&_sizeProbesLock);
    _sizeProbes[type] += (double)size;
    aio4c_mutex_unlock(&_sizeProbesLock);
}

static double _elapsedTime(void) {
    static struct timeval _start;
    static aio4c_bool_t _initialized = false;
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
    aio4c_mutex_lock(&_timeProbesLock);
    pstats("=== %s: %.3f s [%.3f%%]\n", label, _timeProbes[tType] / 1000000.0,
            _timeProbes[tType] * 100.0 / _elapsedTime());
    aio4c_mutex_unlock(&_timeProbesLock);
}

static void _pstats(char* label, ProbeTimeType tType, ProbeSizeType sType) {
    double size = 0.0;
    char* unit = NULL;

    aio4c_mutex_lock(&_sizeProbesLock);
    _ConvertSize(_sizeProbes[sType], &size, &unit);
    aio4c_mutex_unlock(&_sizeProbesLock);
    aio4c_mutex_lock(&_timeProbesLock);
    pstats("=== %s: %.3f %s in %.3f s [%.3f%%]\n", label, size, unit,
            _timeProbes[tType] / 1000000.0,
            _timeProbes[tType] * 100.0 / _elapsedTime());
    aio4c_mutex_unlock(&_timeProbesLock);
}

void _PrintStats(void) {
    static double lastAlloc = 0.0, lastFree = 0.0;
    long _alloc = 0L, _free = 0L;

    aio4c_mutex_lock(&_sizeProbesLock);
    _alloc = (long)_sizeProbes[PROBE_MEMORY_ALLOCATE_COUNT] - lastAlloc;
    lastAlloc = _sizeProbes[PROBE_MEMORY_ALLOCATE_COUNT];
    _free = (long)_sizeProbes[PROBE_MEMORY_FREE_COUNT] - lastFree;
    lastFree = _sizeProbes[PROBE_MEMORY_FREE_COUNT];
    aio4c_mutex_unlock(&_sizeProbesLock);

     pstats("================= STATISTICS ===================%c", '\n');
    _pstats("ALLOCATED MEMORY ", TIME_PROBE_MEMORY_ALLOCATION, PROBE_MEMORY_ALLOCATED_SIZE);
    _pstats("ALLOCATED BUFFERS", TIME_PROBE_BUFFER_ALLOCATION, PROBE_BUFFER_ALLOCATED_SIZE);
    pstats("=== MEMORY ALLOCATION: %ld allocations, %ld frees\n", _alloc, _free);
    _pstats("NETWORK READ     ", TIME_PROBE_NETWORK_READ, PROBE_NETWORK_READ_SIZE);
    _pstats("NETWORK WRITE    ", TIME_PROBE_NETWORK_WRITE, PROBE_NETWORK_WRITE_SIZE);
    _pstats("PROCESSED DATA   ", TIME_PROBE_DATA_PROCESS, PROBE_PROCESSED_DATA_SIZE);
    _pstats("LATENCY          ", TIME_PROBE_LATENCY, PROBE_LATENCY_COUNT);
    _ptimes("IDLE TIME        ", TIME_PROBE_IDLE);
    _ptimes("BLOCKED TIME     ", TIME_PROBE_BLOCK);
    _ptimes("JNI OVERHEAD     ", TIME_PROBE_JNI_OVERHEAD);
     pstats("=== RUNNING THREADS  : %d\n", GetNumThreads());
     pstats("=================    END     ===================%c", '\n');
}

#define floatWithComma(f) (int)(f), (int)(((f) - (double)((int)(f))) * 1000000.0)

void _WriteStats(void) {
    static struct timeval _start;
    static aio4c_bool_t _initialized = false;
    struct timeval time;
    static double lastRead = 0.0, lastWrite = 0.0, lastProcess = 0.0, lastIdle = 0.0, lastLatency = 0.0, lastLatencyCount = 0.0;
    double read = 0.0, write = 0.0, process = 0.0, idle = 0.0, allocated = 0.0, connections = 0.0, latency = 0.0, latencyCount = 0.0;

    if (!_initialized) {
        _initialized = true;
        gettimeofday(&_start, NULL);
    }

    gettimeofday(&time, NULL);

    aio4c_mutex_lock(&_timeProbesLock);
    aio4c_mutex_lock(&_sizeProbesLock);
    read =  _sizeProbes[PROBE_NETWORK_READ_SIZE] - lastRead;
    lastRead = _sizeProbes[PROBE_NETWORK_READ_SIZE];
    write = _sizeProbes[PROBE_NETWORK_WRITE_SIZE] - lastWrite;
    lastWrite = _sizeProbes[PROBE_NETWORK_WRITE_SIZE];
    process = _sizeProbes[PROBE_PROCESSED_DATA_SIZE] - lastProcess;
    lastProcess = _sizeProbes[PROBE_PROCESSED_DATA_SIZE];
    latencyCount = _sizeProbes[PROBE_LATENCY_COUNT] - lastLatencyCount;
    lastLatencyCount = _sizeProbes[PROBE_LATENCY_COUNT];
    idle = _timeProbes[TIME_PROBE_IDLE] - lastIdle;
    lastIdle = _timeProbes[TIME_PROBE_IDLE];
    allocated = _sizeProbes[PROBE_MEMORY_ALLOCATED_SIZE];
    connections = _sizeProbes[PROBE_CONNECTION_COUNT];
    latency = _timeProbes[TIME_PROBE_LATENCY] - lastLatency;
    lastLatency = _timeProbes[TIME_PROBE_LATENCY];
    aio4c_mutex_unlock(&_sizeProbesLock);
    aio4c_mutex_unlock(&_timeProbesLock);

    fprintf(_statsFile, "%u;%d,%d;%d,%d;%d,%d;%d,%d;%d;%d,%d;%d,%d\r\n", (unsigned int)(time.tv_sec - _start.tv_sec),
            floatWithComma(allocated / 1024.0),
            floatWithComma(read / 1024.0), floatWithComma(write / 1024.0), floatWithComma(process / 1024.0), (int)connections,
            floatWithComma(idle / 1000.0), floatWithComma(latencyCount>0?(latency / 1000.0 / latencyCount):0.0));
}

void _StatsEnd(void) {
    if (_statsThread != NULL) {
        _statsThread->state = STOPPED;
        ThreadJoin(_statsThread);
        FreeThread(&_statsThread);
    }

    _PrintStats();
}

