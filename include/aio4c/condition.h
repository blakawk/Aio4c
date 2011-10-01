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
 * @file aio4c/condition.h
 * @brief Provides a portable implementation of POSIX condition variables.
 *
 * This implementation is nevertheless limited to allow only one thread to wait
 * on a condition variable.
 *
 * @author blakawk
 */
#ifndef __AIO4C_CONDITION_H__
#define __AIO4C_CONDITION_H__

#include <aio4c/types.h>

/**
 * @enum ConditionState
 * @brief The state of a Condition.
 *
 * @see Condition
 * @see Thread
 */
/**
 * @var ConditionState AIO4C_COND_STATE_DESTROYED
 * @brief The Condition has been destroyed and should not be used anymore.
 *
 * @see ConditionState
 */
/**
 * @var ConditionState AIO4C_COND_STATE_FREE
 * @brief No Thread is waiting on the Condition.
 *
 * @see ConditionState
 */
/**
 * @var ConditionState AIO4C_COND_STATE_WAITED
 * @brief One Thread is waiting on the Condition.
 *
 * @see ConditionState
 */
/**
 * @var ConditionState AIO4C_COND_STATE_MAX
 * @brief The maximum of ConditionStates.
 *
 * @see ConditionState
 */
typedef enum e_ConditionState {
    AIO4C_COND_STATE_DESTROYED = 0,
    AIO4C_COND_STATE_FREE = 1,
    AIO4C_COND_STATE_WAITED = 2,
    AIO4C_COND_STATE_MAX = 3
} ConditionState;

/**
 * @var ConditionStateString
 * @brief String representation of ConditionStates.
 *
 * @see ConditionState
 */
extern char* ConditionStateString[AIO4C_COND_STATE_MAX];

#ifndef __AIO4C_LOCK_DEFINED__
#define __AIO4C_LOCK_DEFINED__
typedef struct s_Lock Lock;
#endif /* __AIO4C_LOCK_DEFINED__ */

#ifndef __AIO4C_THREAD_DEFINED__
#define __AIO4C_THREAD_DEFINED__
typedef struct s_Thread Thread;
#endif /* __AIO4C_THREAD_DEFINED__ */

/**
 * @struct s_Condition
 * @brief Represents a condition for threads to wait on.
 *
 * A Condition is an holder for implementations of POSIX condition
 * variables in different architectures.
 *
 * @see NewCondition
 */
/**
 * @def __AIO4C_CONDITION_DEFINED__
 * @brief Defined when Condition type is defined.
 */
#ifndef __AIO4C_CONDITION_DEFINED__
#define __AIO4C_CONDITION_DEFINED__
typedef struct s_Condition Condition;
#endif /* __AIO4C_CONDITION_DEFINED__ */

/**
 * @fn Condition* NewCondition(void)
 * @brief Allocates and initializes a Condtion.
 *
 * @return
 *   A pointer to a Condition structure, or NULL if the allocation failed.
 */
extern AIO4C_API Condition* NewCondition(void);

/**
 * @def WaitCondition(condition,lock)
 * @brief Wait on a Condition variable.
 *
 * The calling Thread will wait for the Condition variable to be notified by
 * another Thread. The provided Lock will be atomically released before waiting
 * for the Condition to be notified, and atomically locked when the Condition
 * will be notified.
 *
 * @warning
 *   This functions supports only one Thread at a time.
 *
 * @param condition
 *   A pointer to the Condition to wait on.
 * @param lock
 *   A pointer to the Lock to release atomically. This Lock must be owned by
 *   the calling thread.
 * @return
 *   true if the Condition was notified, false if an error occurred.
 *
 * @see NotifyCondition
 *
 * @fn aio4c_bool_t _WaitCondition(char*,int,Condition*,Lock*)
 * @copydoc WaitCondition(condition,lock)
 *
 * @param file
 *   @see __FILE__
 * @param line
 *   @see __LINE__
 */
#define WaitCondition(condition,lock) \
    _WaitCondition(__FILE__, __LINE__, condition, lock)
extern AIO4C_API aio4c_bool_t _WaitCondition(char* file, int line, Condition* condition, Lock* lock);

/**
 * @def NotifyCondition(condition)
 * @brief Notifies a Thread waiting on a Condition.
 *
 * If a Thread is waiting on a Condition, that Thread will be woken up.
 * If no Thread is waiting, nothing is performed. It is recommended that the
 * Lock associated with the Condition has to be taken before calling this
 * function, in order to predict the ordering of the Thread that will take over
 * the execution once the Condition is notified.
 *
 * @param condition
 *   The condition to notify.
 *
 * @fn void _NotifyCondition(char*,int,Condition*)
 * @copydoc NotifyCondition(condition)
 *
 * @param file
 *   @see __FILE__
 * @param line
 *   @see __LINE__
 */
#define NotifyCondition(condition) \
    _NotifyCondition(__FILE__, __LINE__, condition)
extern AIO4C_API void _NotifyCondition(char* file, int line, Condition* condition);

/**
 * @fn Thread* ConditionGetOwner(Condition*)
 * @brief Retrieves this Condition owner.
 *
 * @param condition
 *   A pointer to the Condition.
 * @return
 *   A pointer to the Thread owner of this Condition or NULL if there is no
 *   owner.
 */
extern AIO4C_API Thread* ConditionGetOwner(Condition* condition);

/**
 * @fn ConditionState ConditionGetState(Condition*)
 * @brief Retrieves this Condition state.
 *
 * @param condition
 *   A pointer to the Condition.
 * @return
 *   The Condition state.
 *
 * @see ConditionState
 */
extern AIO4C_API ConditionState ConditionGetState(Condition* condition);

/**
 * @fn FreeCondition(Condition**)
 * @brief Frees a Condition.
 *
 * If the Condition is actually waited on by a Thread, the behaviour is undefined.
 * The pointed pointer will be set to NULL in order to avoid further accesses.
 *
 * @param condition
 *   A pointer to a Condition's pointer.
 */
extern AIO4C_API void FreeCondition(Condition** condition);

#endif /* __AIO4C_CONDITION_H__ */
