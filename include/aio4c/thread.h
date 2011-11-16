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
/**
 * @file aio4c/thread.h
 * @brief Provides a portable implementation of POSIX threads.
 *
 * @author blakawk
 */
#ifndef __AIO4C_THREAD_H__
#define __AIO4C_THREAD_H__

#include <aio4c/types.h>

#include <stdarg.h>
#include <stdio.h>

/**
 * @typedef ThreadData
 * @brief Pointer to data to be used by a Thread.
 *
 * This pointer is used by Thread routines to be able to pass
 * arbitrary data to be used by Threads.
 */
typedef struct s_ThreadArg* ThreadData;

/**
 * @typedef ThreadInitialization
 * @brief Thread initialization callback.
 *
 * This callback is called by the Thread once it is initialized, but before
 * it is started.
 *
 * It receives a ThreadData as parameter and must return false to indicate
 * that the initialization failed, or true to indicate that initialization
 * succeeded.
 */
typedef bool (*ThreadInitialization)(ThreadData);

/**
 * @typedef ThreadRoutine
 * @brief Thread run callback.
 *
 * This is the routine to be executed by a created Thread.
 *
 * It receives a ThreadData as parameter and should return false
 * when the Thread has to terminates.
 */
typedef bool (*ThreadRoutine)(ThreadData);

/**
 * @typedef ThreadFinalization
 * @brief Thread finalization callback.
 *
 * This callback is called by the Thread once its execution is finished.
 * This is typically called when the ThreadRoutine return false, and used
 * to free any memory used by the Thread.
 */
typedef void (*ThreadFinalization)(ThreadData);

/**
 * @def AIO4C_DEBUG_THREADS
 * @brief Set to 1 if threads functions are being debugged.
 */
#ifndef AIO4C_DEBUG_THREADS
#define AIO4C_DEBUG_THREADS 0
#endif

/**
 * @def dthread
 * @brief Used to print thread debugging information on stderr.
 */
#define dthread(fmt,...) {                 \
    if (AIO4C_DEBUG_THREADS) {             \
        fprintf(stderr, fmt, __VA_ARGS__); \
    }                                      \
}

/**
 * @enum ThreadState
 * @brief Represents different states of a Thread.
 */
typedef enum e_ThreadState {
    AIO4C_THREAD_STATE_NONE = 0,    /**< Not initialized. */
    AIO4C_THREAD_STATE_RUNNING = 1, /**< Running */
    AIO4C_THREAD_STATE_BLOCKED = 2, /**< Waiting to acquire a Lock */
    AIO4C_THREAD_STATE_IDLE = 3,    /**< Waiting on a Condition */
    AIO4C_THREAD_STATE_EXITED = 4,  /**< Execution ended */
    AIO4C_THREAD_STATE_JOINED = 5,  /**< Freed */
    AIO4C_THREAD_STATE_MAX = 6      /**< The number of ThreadStates */
} ThreadState;

/**
 * @var ThreadStateString
 * @brief String representation of ThreadStates.
 *
 * @see ThreadState
 */
extern char* ThreadStateString[AIO4C_THREAD_STATE_MAX];

/**
 * @def __AIO4C_THREAD_DEFINED__
 * @brief Defined when Thread type has been defined.
 */
/**
 * @struct s_Thread
 * @brief Represents a thread.
 *
 * This structure holds information needed in order to manage Threads.
 * A Thread is a light-weight process that perform an infinite loop
 * running a custom function (named routine in this implementation).
 *
 * It can uses Locks and Conditions in order to interact with other Threads.
 *
 * This implementation tries to be as OS-independant as possible.
 */
#ifndef __AIO4C_THREAD_DEFINED__
#define __AIO4C_THREAD_DEFINED__
typedef struct s_Thread Thread;
#endif /* __AIO4C_THREAD_DEFINED__ */

/**
 * @def __AIO4C_LOCK_DEFINED__
 * @brief Defined when Lock type has been defined.
 */
#ifndef __AIO4C_LOCK_DEFINED__
#define __AIO4C_LOCK_DEFINED__
typedef struct s_Lock Lock;
#endif /* __AIO4C_LOCK_DEFINED__ */

/**
 * @def __AIO4C_CONDITION_DEFINED__
 * @brief Defined when Condition type has been defined.
 */
#ifndef __AIO4C_CONDITION_DEFINED__
#define __AIO4C_CONDITION_DEFINED__
typedef struct s_Condition Condition;
#endif /* __AIO4C_CONDITION_DEFINED__ */

/**
 * @fn void ThreadMain(void)
 * @brief Defines the current thread as main thread.
 *
 * Initializes structures used for Thread debugging.
 */
extern AIO4C_API void ThreadMain(void);

/**
 * @fn int GetNumThreads(void)
 * @brief Retrieves the number of Threads actually running.
 *
 * @return
 *   The number of created Threads.
 */
extern AIO4C_API int GetNumThreads(void);

/**
 * @fn Thread* NewThread(char*,ThreadInitialization,ThreadRoutine,ThreadFinalization,ThreadData)
 * @brief Creates a Thread.
 *
 * Allocates the data structure used by Thread.
 *
 * @param name
 *   Name of the created thread.
 * @param initialize
 *   Thread initialization callback.
 * @param routine
 *   Thread routine callback.
 * @param finalize
 *   Thread finalization callback.
 * @param data
 *   Pointer to data to be passed to the callbacks.
 * @return
 *   A pointer to the newly created Thread.
 */
extern AIO4C_API Thread* NewThread(char* name, ThreadInitialization initialize, ThreadRoutine routine, ThreadFinalization finalize, ThreadData data);

/**
 * @fn char* ThreadGetName(Thread*)
 * @brief Retrieves a Thread's name.
 *
 * @param thread
 *   Pointer to a Thread structure.
 * @return
 *   The Thread's name.
 */
extern AIO4C_API char* ThreadGetName(Thread* thread);

/**
 * @fn int ThreadGetId(Thread*)
 * @brief Retrieves a Thread's identifier.
 *
 * @param thread
 *   Pointer to a Thread structure.
 * @return
 *   The Thread's identifier.
 */
extern AIO4C_API unsigned long ThreadGetId(Thread* thread);

/**
 * @fn ThreadState ThreadGetState(Thread*)
 * @brief Retrieves a Thread's state.
 *
 * @param thread
 *   Pointer to a Thread structure.
 * @return
 *   The Thread's state.
 */
extern AIO4C_API ThreadState ThreadGetState(Thread* thread);

/**
 * @fn void ThreadSetState(Thread*,ThreadState)
 * @brief Sets a Thread's state.
 *
 * @param thread
 *   Pointer to a Thread structure.
 * @param state
 *   The new ThreadState to set.
 */
extern AIO4C_API void ThreadSetState(Thread* thread, ThreadState state);

/**
 * @fn void ThreadStop(Thread*)
 * @brief Requires a Thread to terminate.
 *
 * This function requires a thread to exits its infinite loop.
 *
 * @param thread
 *   Pointer to a Thread structure.
 */
extern AIO4C_API void ThreadStop(Thread* thread);

/**
 * @fn bool ThreadStart(Thread*)
 * @brief Starts a Thread.
 *
 * Creates the thread using OS-dependant functions. The created thread will then
 * call the initialization callback, and this function will wait until the thread
 * is initialized. This function returns only once the thread has been initialized.
 *
 * Once initialized, the thread enters an infinite loop calling the thread's routine until it
 * returns false, thus indicating that the thread should terminate.
 *
 * When the thread is terminated, the finalization callback is called and the thread wait to
 * be joined.
 *
 * @param thread
 *   Pointer to a Thread structure.
 * @return
 *   true if the Thread has been started with success, else false.
 */
extern AIO4C_API bool ThreadStart(Thread* thread);

/**
 * @fn bool ThreadRunning(Thread*)
 * @brief Determines if a Thread is running.
 *
 * @param thread
 *   Pointer to a Thread structure.
 * @return
 *   true if the Thread is running, false if not.
 */
extern AIO4C_API bool ThreadRunning(Thread* thread);

/**
 * @def ThreadSelf
 * @brief Macros linking to _ThreadSelf function.
 *
 * This macros links to _ThreadSelf if threads debugging is enabled,
 * else translate to NULL.
 *
 * @see AIO4C_DEBUG_THREADS
 */
/**
 * @fn Thread* _ThreadSelf(void)
 * @brief Retrieves the current thread's control structure pointer.
 *
 * Tries to retrieve the current Thread's information according to
 * its identifier.
 *
 * @warning This is performance consuming.
 *
 * @return
 *   Pointer to the Thread structure of the caller Thread, or NULL if
 *   it cannot be determined.
 */
#if AIO4C_DEBUG_THREADS
#define ThreadSelf() _ThreadSelf()
#else /* AIO4C_DEBUG_THREADS */
#define ThreadSelf() NULL
#endif /* AIO4C_DEBUG_THREADS */
extern AIO4C_API Thread* _ThreadSelf(void);

/**
 * @fn void ThreadJoin(Thread*)
 * @brief Joins a Thread.
 *
 * If the Thread is running, as determined by ThreadRunning function,
 * the underlying thread joining function is called, meaning that the
 * calling thread will block until the joined thread terminates.
 *
 * Once the thread is joined, its ressources are freed using FreeThread.
 *
 * @param thread
 *   The Thread to join.
 *
 * @see void FreeThread(Thread**)
 * @see bool ThreadRunning(Thread*)
 */
extern AIO4C_API void ThreadJoin(Thread* thread);

/**
 * @fn void FreeThread(Thread**)
 * @brief Frees a Thread.
 *
 * Frees data used by a Thread. IF the Thread is running, undefined behaviour
 * can happen. The pointer pointer is then set to NULL to avoid further use.
 *
 * @param thread
 *   Pointer to a pointer to a Thread structure.
 */
extern AIO4C_API void FreeThread(Thread** thread);

#endif /* __AIO4C_THREAD_H__ */
