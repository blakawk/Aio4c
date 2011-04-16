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
#ifndef __AIO4C_CONDITION_H__
#define __AIO4C_CONDITION_H__

#include <aio4c/types.h>

#ifndef AIO4C_WIN32
#include <pthread.h>
#else /* AIO4C_WIN32 */
#include <winbase.h>
#endif /* AIO4C_WIN32 */

typedef enum e_ConditionState {
    AIO4C_COND_STATE_DESTROYED,
    AIO4C_COND_STATE_FREE,
    AIO4C_COND_STATE_WAITED,
    AIO4C_COND_STATE_MAX
} ConditionState;

extern char* ConditionStateString[AIO4C_COND_STATE_MAX];

#ifndef __AIO4C_LOCK_DEFINED__
#define __AIO4C_LOCK_DEFINED__
typedef struct s_Lock Lock;
#endif /* __AIO4C_LOCK_DEFINED__ */

#ifndef __AIO4C_THREAD_DEFINED__
#define __AIO4C_THREAD_DEFINED__
typedef struct s_Thread Thread;
#endif /* __AIO4C_THREAD_DEFINED__ */

#ifndef __AIO4C_CONDITION_DEFINED__
#define __AIO4C_CONDITION_DEFINED__
typedef struct s_Condition Condition;
#endif /* __AIO4C_CONDITION_DEFINED__ */

struct s_Condition {
    ConditionState state;
    Thread*        owner;
#ifndef AIO4C_WIN32
    pthread_cond_t     condition;
#else /* AIO4C_WIN32 */
#ifdef AIO4C_HAVE_CONDITION
    CONDITION_VARIABLE condition;
#else /* AIO4C_HAVE_CONDITION */
    HANDLE             mutex;
    HANDLE             event;
#endif /* AIO4C_HAVE_CONDITION */
#endif /* AIO4C_WIN32 */
};

extern AIO4C_API Condition* NewCondition(void);

#define WaitCondition(condition,lock) \
    _WaitCondition(__FILE__, __LINE__, condition, lock)
extern AIO4C_API aio4c_bool_t _WaitCondition(char* file, int line, Condition* condition, Lock* lock);

#define NotifyCondition(condition) \
    _NotifyCondition(__FILE__, __LINE__, condition)
extern AIO4C_API void _NotifyCondition(char* file, int line, Condition* condition);

extern AIO4C_API void FreeCondition(Condition** condition);

#endif
