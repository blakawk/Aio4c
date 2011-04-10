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
#ifndef __AIO4C_LOCK_H__
#define __AIO4C_LOCK_H__

#include <aio4c/types.h>

#ifndef AIO4C_WIN32
#include <pthread.h>
#else /* AIO4C_WIN32 */
#include <winbase.h>
#endif /* AIO4C_WIN32 */

typedef enum e_LockState {
    AIO4C_LOCK_STATE_DESTROYED,
    AIO4C_LOCK_STATE_FREE,
    AIO4C_LOCK_STATE_LOCKED,
    AIO4C_LOCK_STATE_MAX
} LockState;

extern char* LockStateString[AIO4C_LOCK_STATE_MAX];

#ifndef __AIO4C_LOCK_DEFINED__
#define __AIO4C_LOCK_DEFINED__
typedef struct s_Lock Lock;
#endif /* __AIO4C_LOCK_DEFINED__ */

#ifndef __AIO4C_THREAD_DEFINED__
#define __AIO4C_THREAD_DEFINED__
typedef struct s_Thread Thread;
#endif /* __AIO4C_THREAD_DEFINED__ */

struct s_Lock {
    LockState        state;
    Thread*          owner;
#ifndef AIO4C_WIN32
    pthread_mutex_t  mutex;
#else /* AIO4C_WIN32 */
    CRITICAL_SECTION mutex;
#endif /* AIO4C_WIN32 */
};

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

extern Lock* NewLock(void);

#define TakeLock(lock) \
    _TakeLock(__FILE__, __LINE__, lock)
extern Lock* _TakeLock(char* file, int line, Lock* lock);

#define ReleaseLock(lock) \
    _ReleaseLock(__FILE__, __LINE__, lock)
extern Lock* _ReleaseLock(char* file, int line, Lock* lock);

extern void FreeLock(Lock** lock);

#endif
