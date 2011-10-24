/*
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
 * @file aio4c/lock.h
 * @brief Provides a POSIX platform independant implementation of mutexes.
 *
 * @author blakawk
 */
#ifndef __AIO4C_LOCK_H__
#define __AIO4C_LOCK_H__

#include <aio4c/types.h>

#ifndef AIO4C_WIN32
#include <pthread.h>
#else /* AIO4C_WIN32 */
#include <winbase.h>
#endif /* AIO4C_WIN32 */

/**
 * @enum LockState
 * @brief Represents the states of a Lock.
 */
typedef enum e_LockState {
    AIO4C_LOCK_STATE_DESTROYED = 0, /**< The Lock has been freed. */
    AIO4C_LOCK_STATE_FREE = 1,      /**< The Lock is not taken. */
    AIO4C_LOCK_STATE_LOCKED = 2,    /**< The Lock is taken. */
    AIO4C_LOCK_STATE_MAX = 3        /**< Maximum number of Lock states. */
} LockState;

/**
 * @var LockStateString
 * @brief String representation of LockState.
 *
 * @see LockState
 */
extern char* LockStateString[AIO4C_LOCK_STATE_MAX];

/**
 * @struct s_Lock
 * @brief Represents a Lock.
 *
 * A Lock is defined to have:
 * <ul>
 *   <li>a state,</li>
 *   <li>an owner when its state is locked,</li>
 *   <li>the underlying platform dependant mutex structure.</li>
 * </ul>
 */
/**
 * @def __AIO4C_LOCK_DEFINED__
 * @brief Defined if Lock type has been defined.
 */
#ifndef __AIO4C_LOCK_DEFINED__
#define __AIO4C_LOCK_DEFINED__
typedef struct s_Lock Lock;
#endif /* __AIO4C_LOCK_DEFINED__ */

#ifndef __AIO4C_THREAD_DEFINED__
#define __AIO4C_THREAD_DEFINED__
typedef struct s_Thread Thread;
#endif /* __AIO4C_THREAD_DEFINED__ */

/**
 * @def AIO4C_LOCK_INITIALIZER
 * @brief Static Lock initializer.
 *
 * Initializes a Lock structure.
 */
#ifndef AIO4C_WIN32
#define AIO4C_LOCK_INITIALIZER {       \
    .state = AIO4C_LOCK_STATE_NONE,    \
    .owner = NULL,                     \
    .mutex = PTHREAD_MUTEX_INITIALIZER \
}
#else /* AIO4C_WIN32 */
#define AIO4C_LOCK_INITIALIZER {      \
    .state = AIO4C_LOCK_STATE_NONE,   \
    .owner = NULL                     \
}
#endif /* AIO4C_WIN32 */

/**
 * @fn Lock* NewLock(void)
 * @brief Allocates a Lock.
 *
 * Upon allocation, a Lock will be in state FREE and without owner.
 *
 * @return
 *   A pointer to the allocated Lock.
 */
extern AIO4C_API Lock* NewLock(void);

/**
 * @def TakeLock(lock)
 * @brief _TakeLock wrapper for debugging purpose.
 *
 * Call _TakeLock function using __FILE__ and __LINE__ compiler's
 * macros for debugging purpose.
 *
 * @see _TakeLock(char*,int,Lock*)
 */
/**
 * @fn Lock* _TakeLock(char*,int,Lock*)
 * @brief Takes a Lock.
 *
 * Once this function returns, it is guaranteed that only one Thread now own the
 * Lock, meaning that any other Thread trying to take the lock will be blocked until
 * the owner releases it.
 *
 * If a Thread is blocked because it cannot acquire the Lock, then its state is changed
 * to BLOCKED, and BLOCK statistic is gathered.
 *
 * When the Lock is acquired, the Lock state switches to LOCKED, and the current
 * Thread is set as the Lock owner.
 *
 * @param file
 *   The source file name where the function is called (normally __FILE__ macro).
 * @param line
 *   The line where the function is called (normally __LINE__ macro).
 * @param lock
 *   Pointer to the Lock to take.
 * @return
 *   Pointer to the taken Lock.
 */
#define TakeLock(lock) \
    _TakeLock(__FILE__, __LINE__, lock)
extern AIO4C_API Lock* _TakeLock(char* file, int line, Lock* lock);

/**
 * @def ReleaseLock(lock)
 * @brief _ReleaseLock wrapper for debugging purpose.
 *
 * Call _ReleaseLock function using __FILE__ and __LINE__ compiler's
 * macros for debugging purpose.
 *
 * @see Lock* _ReleaseLock(char*,int,Lock*)
 */
/**
 * @fn Lock* _ReleaseLock(char*,int,Lock*)
 * @brief Releases a Lock.
 *
 * The Lock is released. If the Lock was not taken by any thread, then undefined
 * behaviour is expected, as well as if the Lock is not taken by the calling
 * Thread.
 *
 * @param file
 *   The source file name where the function is called (normally __FILE__ macro).
 * @param line
 *   The line where the function is called (normally __LINE__ macro).
 * @param lock
 *   Pointer to the Lock to release.
 * @return
 *   Pointer to the released Lock.
 */
#define ReleaseLock(lock) \
    _ReleaseLock(__FILE__, __LINE__, lock)
extern AIO4C_API Lock* _ReleaseLock(char* file, int line, Lock* lock);

/**
 * @fn Lock* LockSetOwner(Lock*,Thread*)
 * @brief Sets the Lock's owner.
 *
 * @param lock
 *   Pointer to the Lock to set the owner.
 * @param owner
 *   Pointer to the new Lock's owner.
 * @return
 *   Pointer to the Lock.
 */
extern AIO4C_API Lock* LockSetOwner(Lock* lock, Thread* owner);

/**
 * @fn Thread* LockGetOwner(Lock*)
 * @brief Gets the current Lock's owner.
 *
 * @param lock
 *   Pointer to the Lock to retrieve the owner from.
 * @return
 *   NULL if the Lock is FREE, else the current Lock's owner.
 */
extern AIO4C_API Thread* LockGetOwner(Lock* lock);

/**
 * @fn Lock* LockSetState(Lock*,LockState)
 * @brief Sets the Lock's state.
 *
 * @param lock
 *   Pointer to the Lock.
 * @param state
 *   The new Lock's state.
 * @return
 *   Pointer to the Lock.
 */
extern AIO4C_API Lock* LockSetState(Lock* lock, LockState state);

/**
 * @fn LockState LockGetState(Lock*)
 * @brief Gets the current Lock's state.
 *
 * @param lock
 *   Pointer to the Lock to retrieve the state from.
 * @return
 *   The current lock's state.
 */
extern AIO4C_API LockState LockGetState(Lock* lock);

/**
 * @fn void* LockGetMutex(Lock*)
 * @brief Gets pointer to the Lock underlying platform dependant structure.
 *
 * @param lock
 *   Pointer to the Lock.
 * @return
 *   Pointer to the platform dependant mutex's structure.
 */
extern AIO4C_API void* LockGetMutex(Lock* lock);

/**
 * @fn void FreeLock(Lock**)
 * @brief Frees a Lock.
 *
 * The Lock should be allocated using NewLock function.
 * If the Lock is taken when the free occurs, undefined behaviour is expected.
 * When free is done, the pointed pointer is set to NULL in order to disallow
 * further use.
 *
 * @param lock
 *   A pointer to a Lock pointer.
 */
extern AIO4C_API void FreeLock(Lock** lock);

#endif /* __AIO4C_LOCK_H__ */
