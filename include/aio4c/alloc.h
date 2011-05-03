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
 * @file aio4c/alloc.h
 * @brief Provides memory allocation functions.
 *
 * @author blakawk
 */
/**
 * @def __AIO4C_ALLOC_H__
 * @brief Defined if aio4c/alloc.h has been included.
 *
 * This modules is provided to collect statistics about memory allocation in
 * the library.
 *
 * @see aio4c/alloc.h
 */
#ifndef __AIO4C_ALLOC_H__
#define __AIO4C_ALLOC_H__

#include <aio4c/types.h>

/**
 * @fn void* aio4c_malloc(int)
 * @brief Allocates memory on dynamic heap.
 *
 * In addition to allocating memory, this function collect statistics about
 * memory allocation using stats module. In order to track memory allocation,
 * it allocates four bytes in addition to the requested size, to store the
 * allocated size. The returned pointer "hides" this overhead allocation.
 *
 * @param size
 *   The memory zone size to allocate, in bytes.
 * @return
 *   A pointer to the allocated memory, or NULL if memory allocation failed.
 *
 * @see aio4c/stats.h
 */
extern AIO4C_API void* aio4c_malloc(int size);

/**
 * @fn void* aio4c_realloc(void*,size)
 * @brief Extend or shrink allocated memory.
 *
 * As in aio4c_malloc(int), this function collects statistics about memory
 * allocation and stores the size in front of allocated memory. The returned
 * pointer "hides" this overhead allocation.
 *
 * @param ptr
 *   A pointer to the previously allocated memory by aio4c_malloc/realloc
 *   functions. If NULL, then the call will be equivalent to:
 *   @code
 *     aio4c_malloc(size)
 *   @endcode
 *
 * @param size
 *   The requested size of the memory zone. If it is less than previously
 *   allocated zone size, then the overhead is free'd. If it is 0, then the
 *   call will be equivalent to:
 *   @code
 *     aio4c_free(ptr)
 *   @endcode
 *
 * @return
 *   A pointer to the new allocated memory, or NULL if re-allocation failed.
 */
extern AIO4C_API void* aio4c_realloc(void* ptr, int size);

/**
 * @fn void aio4c_free(void*)
 * @brief Frees a previously allocated memory.
 *
 * This function frees the allocated memory pointed by ptr. The ptr MUST be
 * a pointer to a zone previously returned by aio4c_malloc/realloc functions,
 * to correctly collect statistics about memory allocation. If the ptr passed
 * to this function was not returned by aio4c_malloc/realloc functions, bad
 * things can occur.
 *
 * @param ptr
 *   A pointer to an allocated memory zone by aio4c_malloc/realloc functions.
 */
extern AIO4C_API void aio4c_free(void* ptr);

#endif /* __AIO4C_ALLOC_H__ */
