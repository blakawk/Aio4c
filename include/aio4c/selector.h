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
 * @file aio4c/selector.h
 * @brief Provides Selector.
 *
 * A Selector allows a Thread to wait for an i/o operation
 * on several file descriptors.
 *
 * @author blakawk
 */
#ifndef __AIO4C_SELECTOR_H__
#define __AIO4C_SELECTOR_H__

#include <aio4c/list.h>
#include <aio4c/lock.h>
#include <aio4c/types.h>

#ifndef AIO4C_WIN32
#include <poll.h>
#endif /* AIO4C_WIN32 */

/**
 * @def __AIO4C_LOCK_DEFINED__
 * @brief Defined when Lock type has been defined.
 */
#ifndef __AIO4C_LOCK_DEFINED__
#define __AIO4C_LOCK_DEFINED__
typedef struct s_Lock Lock;
#endif /* __AIO4C_LOCK_DEFINED__ */

/**
 * @enum SelectionOperation
 * @brief Type of operation to wait on a Selector for.
 */
typedef enum e_SelectionOperation {
#ifdef AIO4C_HAVE_POLL
#ifndef AIO4C_WIN32
    AIO4C_OP_READ = POLLIN,      /**< The operation to be notified for is a network read */
    AIO4C_OP_WRITE = POLLOUT,    /**< The operation to be notified for is a network write */
#else /* AIO4C_WIN32 */
    AIO4C_OP_READ = POLLRDNORM,  /**< The operation to be notified for is a network read */
    AIO4C_OP_WRITE = POLLWRNORM, /**< The operation to be notified for is a network write */
#endif /* AIO4C_WIN32 */
    AIO4C_OP_ERROR = POLLERR     /**< An error occurred (output only) */
#else /* AIO4C_HAVE_POLL */
    AIO4C_OP_READ  = 0x01,       /**< The operation to be notified for is a network read */
    AIO4C_OP_WRITE = 0x02,       /**< The operation to be notified for is a network write */
    AIO4C_OP_ERROR = 0x04        /**< An error occurred (output only) */
#endif /* AIO4C_HAVE_POLL */
} SelectionOperation;

/**
 * @struct s_SelectionKey
 * @brief Result of a Select operation.
 *
 * A SelectionKey holds the information providing the status
 * of a requested Select operation on a Selector such as the
 * file descriptor that triggered the notification, the operation
 * result, and some internal needs.
 *
 * A SelectionKey is also able to hold a generic pointer to some
 * user data to be associated with the file descriptor, in order
 * to retrieve some context procesing the notification.
 */
/**
 * @def __AIO4C_SELECTION_KEY_DEFINED__
 * @brief Defined when the SelectionKey type is defined.
 */
#ifndef __AIO4C_SELECTION_KEY_DEFINED__
#define __AIO4C_SELECTION_KEY_DEFINED__
typedef struct s_SelectionKey SelectionKey;
#endif /* __AIO4C_SELECTION_KEY_DEFINED__ */

/**
 * @struct s_Selector
 * @brief Holds select operation data.
 *
 * A Selector allows a Thread to wait on several file descriptors
 * (helds by a SelectionKey) for a SelectionOperation notification
 * from the underlying operation system.
 *
 * The preferred system call used is poll operation, but if it is not
 * available, then a select is used. Also, a Selector uses a pipe to be
 * able to wake-up a Thread waiting on a Selector in order to be able to
 * add another SelectionKey.
 *
 * On operating systems on which there is no anonymous pipe availability
 * (such as Windows-based ones), or even where select or poll operations
 * can only be performed on sockets, an UDP socket will be used instead.
 */
/**
 * @def __AIO4C_SELECTOR_DEFINED__
 * @brief Defined when the Selector type is defined.
 */
#ifndef __AIO4C_SELECTOR_DEFINED__
#define __AIO4C_SELECTOR_DEFINED__
typedef struct s_Selector Selector;
#endif /* __AIO4C_SELECTOR_DEFINED__ */

/**
 * @fn aio4c_socket_t SelectionKeyGetSocket(SelectionKey*)
 * @brief Retrieves the socket file descriptor associated with a SelectionKey.
 *
 * @param key
 *   Pointer to the SelectionKey
 * @return
 *   The socket's file descriptor
 */
extern AIO4C_API aio4c_socket_t SelectionKeyGetSocket(SelectionKey* key);

/**
 * @fn void* SelectionKeyGetAttachment(SelectionKey*);
 * @brief Retrieves the user data pointer associated with a SelectionKey.
 *
 * @param key
 *   Pointer to the SelectionKey
 * @return
 *   Pointer to the associated user data
 */
extern AIO4C_API void* SelectionKeyGetAttachment(SelectionKey* key);

/**
 * @fn SelectionOperation SelectionKeyGetOperation(SelectionKey*)
 * @brief Retrieves the SelectionOperation of a SelectionKey.
 *
 * @param key
 *   Pointer to the SelectionKey
 * @return
 *   The SelectionOperation of the SelectionKey
 */
extern AIO4C_API SelectionOperation SelectionKeyGetOperation(SelectionKey* key);

/**
 * @fn SelectionOperation SelectionKeyGetResult(SelectionKey*)
 * @brief Retrieves the select operation result.
 *
 * @param key
 *   Pointer to the SelectionKey
 * @return
 *   The operation result
 */
extern AIO4C_API SelectionOperation SelectionKeyGetResult(SelectionKey* key);

/**
 * @fn bool SelectionKeyIsOperationSuccessful(SelectionKey*)
 * @brief Determines if the operation handled by a SelectionKey was a success.
 *
 * @param key
 *   Pointer to the SelectionKey
 * @return
 *   true if the operation was a success, else false
 */
extern AIO4C_API bool SelectionKeyIsOperationSuccessful(SelectionKey* key);

/**
 * @fn int SelectionKeyGetCurrentCount(SelectionKey*)
 * @brief Retrieves the current count of processed notifications.
 *
 * @param key
 *   Pointer to the SelectionKey
 * @return
 *   The count of processed notifications.
 */
extern AIO4C_API int SelectionKeyGetCurrentCount(SelectionKey* key);

/**
 * @fn int SelectionKeyGetRegistrationCount(SelectionKey*)
 * @brief Retrieves the number of registrations.
 *
 * @param key
 *   Pointer to the SelectionKey
 * @return
 *   The count of registration
 */
extern AIO4C_API int SelectionKeyGetRegistrationCount(SelectionKey* key);

/**
 * @fn Selector* NewSelector(void)
 * @brief Allocates a Selector.
 *
 * @return
 *   Pointer to the allocated Selector, or NULL if it failed.
 */
extern AIO4C_API Selector* NewSelector(void);

/**
 * @fn int SelectorGetPollsNumber(Selector*)
 * @brief Retrieves the Selector's count of registered polls.
 *
 * @param selector
 *   Pointer to the Selector
 * @return
 *   The count of registered polls
 */
extern AIO4C_API int SelectorGetPollsNumber(Selector* selector);

/**
 * @fn int SelectorGetPollsMax(Selector*)
 * @brief Retrieves the maximum of polls that can be registered.
 *
 * @param selector
 *   Pointer to the Selector
 * @return
 *   The maximum number of allowed polls registration
 */
extern AIO4C_API int SelectorGetPollsMax(Selector* selector);

#ifndef AIO4C_HAVE_PIPE

/**
 * @fn aio4c_port_t SelectorGetPort(Selector*)
 * @brief Retrieves the UDP port number associated with the Selector.
 *
 * This function is defined only on platforms where anonymous pipes are
 * not available.
 *
 * @param selector
 *   Pointer to the Selector
 * @return
 *   The UDP port number used by the Selector
 */
extern AIO4C_API aio4c_port_t SelectorGetPort(Selector* selector);

#endif /* AIO4C_HAVE_PIPE */

/**
 * @fn SelectionKey* SelectorGetCurrentKey(Selector*)
 * @brief Retrieves the current SelectionKey.
 *
 * This function is to be called only when iterating over the set of ready
 * SelectionKeys using SelectionKeyReady.
 *
 * @param selector
 *   Pointer to the Selector
 * @return
 *   Pointer to the current ready SelectionKey of the Selector
 */
extern AIO4C_API SelectionKey* SelectorGetCurrentKey(Selector* selector);

/**
 * @fn SelectionKey* Register(Selector*,SelectionOperation,aio4c_socket_t,void*)
 * @brief Registers a socket to a Selector.
 *
 * The Register operation allows one to tell that for next select operation, a Thread
 * should be notified of the operation on the socket, and provide the user data pointed
 * by attachment.
 *
 * This operation provides a SelectionKey to the User in order to be able allow unregistration.
 *
 * @param selector
 *   Pointer to the Selector
 * @param operation
 *   Required operation to be notified for
 * @param fd
 *   The file descriptor of the socket to wait on
 * @param attachment
 *   Pointer to the user data to associate with the SelectionKey
 * @return
 *   Pointer to the associated SelectionKey
 */
extern AIO4C_API SelectionKey* Register(Selector* selector, SelectionOperation operation, aio4c_socket_t fd, void* attachment);

/**
 * @fn void Unregister(Selector*,SelectionKey*,bool,bool*)
 * @brief Allows to unregister a SelectionKey from a Selector.
 *
 * The Unregister operation will allow the user to tell that it is no more interested
 * in receiving notifications for the provided SelectionKey.
 *
 * @param selector
 *   Pointer to the Selector
 * @param key
 *   Pointer to the SelectionKey. The SelectionKey must be one provided by a previous Register operation
 *   on the same selector, if not, then undefined behavior can happen
 * @param unregisterAll
 *   If several registrations has been performed with the same socket associated with the SelectionKey, then
 *   this boolean control whether all the SelectionKeys associated with that socket are unregistered or not
 * @param isLastRegistration
 *   Pointer to a bool variable. The destination variable will be set to true if the unregistered key is the
 *   last one associated with the socket for the concerned selector.
 */
extern AIO4C_API void Unregister(Selector* selector, SelectionKey* key, bool unregisterAll, bool* isLastRegistration);

/**
 * @def Select(selector)
 * @brief Wrapper to the _Select operation.
 *
 * Provides the _Select operation with the special preprocessor macros __FILE__ and __LINE__.
 *
 * @param selector
 *   Pointer to the Selector
 *
 * @see int _Select(Selector*)
 */
#define Select(selector) \
    _Select(__FILE__, __LINE__, selector)

/**
 * @fn int _Select(char*,int,Selector*)
 * @brief Performs a Select operation.
 *
 * Using the related system call, the _Select operation will sleep the calling
 * Thread until the underlying operating system notifies it about one or more
 * operation that could be performed without blocking on all registered sockets,
 * including errors.
 *
 * In order to retrieve the list of SelectionKey for which the Select operation
 * was notified, one can use SelectionKeyReady in order to go through the ready
 * keys.
 *
 * @param file
 *   File name where the function is called (usually __FILE__ macro)
 * @param line
 *   Line number where the function is called (usually __LINE__ macro)
 * @param selector
 *   Pointer to the Selector
 * @return
 *   The number of keys for which an operation is available
 */
extern AIO4C_API int _Select(char* file, int line, Selector* selector);

/**
 * @def SelectorWakeUp(selector)
 * @brief Wrapper to the _SelectorWakeUp function.
 *
 * Expands to _SelectorWakeUp function providing __FILE__ and __LINE__ macros.
 *
 * @see void _SelectorWakeUp(char*,int,Selector*)
 */
#define SelectorWakeUp(selector) \
    _SelectorWakeUp(__FILE__, __LINE__, selector)

/**
 * @fn void _SelectorWakeUp(char*,int,Selector*)
 * @brief Wakes up a Thread waiting on a Selector.
 *
 * @param file
 *   Filename where the function was called (usually __FILE__ macro)
 * @param line
 *   Line number where the function was called (usually __LINE__ macro)
 * @param selector
 *   Pointer to the Selector to wake up
 */
extern AIO4C_API void _SelectorWakeUp(char* file, int line, Selector* selector);

/**
 * @fn bool SelectionKeyReady(Selector*,SelectionKey**)
 * @brief Allows to go through the ready keys of a select operation.
 *
 * If no Select operation has been performed before calling this function,
 * or if the Select operation returned zero, no SelectionKey will be returned,
 * else the Selector's set of ready keys will be iterated once.
 *
 * @param selector
 *   Pointer to the Selector
 * @param key
 *   Pointer to a variable that will be set to a pointer to a SelectionKey, or NULL
 *   if no SelectionKey has been found.
 * @return
 *   true if a SelectionKey has been found, false if no more SelectionKey are available
 *   and a Select operation can be performed again on the Selector.
 */
extern AIO4C_API bool SelectionKeyReady(Selector* selector, SelectionKey** key);

/**
 * @fn void FreeSelector(Selector**)
 * @brief Frees a Selector.
 *
 * Frees the memory allocated for a Selector. Once freed, a Selector cannot be used
 * anymore. If the Selector is still in use by a Thread when this function is called,
 * undefined behavior can occur.
 *
 * @param selector
 *   Pointer to a pointer to a Selector to be freed. Set to NULL once the free
 *   operation succeeds.
 */
extern AIO4C_API void FreeSelector(Selector** selector);

#endif /* __AIO4C_SELECTOR_H__ */
