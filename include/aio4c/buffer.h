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
 * @file aio4c/buffer.h
 * @brief Provides functions to handle byte buffers.
 *
 * @author blakawk
 */
#ifndef __AIO4C_BUFFER_H__
#define __AIO4C_BUFFER_H__

#include <aio4c/queue.h>
#include <aio4c/types.h>

#include <string.h>

/**
 * @def AIO4C_BUFFER_POOL_BATCH_SIZE
 * @brief Number of Buffers allocated when a BufferPool is exhausted.
 *
 * Default value: 16
 *
 * @see BufferPool
 */
#ifndef AIO4C_BUFFER_POOL_BATCH_SIZE
#define AIO4C_BUFFER_POOL_BATCH_SIZE 16
#endif /* AIO4C_BUFFER_POOL_BATCH_SIZE */

/**
 * @struct s_Buffer
 * @brief Represents a byte Buffer.
 *
 * A byte Buffer holds a fixed cound of bytes (his capacity), as well as a
 * current position and a limit. The limit is used to determine which part of
 * the Buffer contains real data.
 *
 * The main purpose of a Buffer is to be used in I/O operation to/from the
 * network.
 *
 * @see ConnectionRead(Connection*)
 * @see ConnectionProcessData(Connection*)
 * @see ConnectionWrite(Connection*)
 */
/**
 * @def __AIO4C_BUFFER_DEFINED__
 * @brief Defined if Buffer type has been defined.
 *
 * @see Buffer
 */
#ifndef __AIO4C_BUFFER_DEFINED__
#define __AIO4C_BUFFER_DEFINED__
typedef struct s_Buffer Buffer;
#endif /* __AIO4C_BUFFER_DEFINED__ */

/**
 * @struct s_BufferPool
 * @brief Represents a pool of Buffers.
 *
 * Buffer pools allows to manage dynamic Buffers allocation in order to reduce
 * overhead due to memory allocation.
 *
 * @see Buffer
 */
/**
 * @def __AIO4C_BUFFER_POOL_DEFINED__
 * @brief Defined when BufferPool type has been defined.
 *
 * @see BufferPool
 */
#ifndef __AIO4C_BUFFER_POOL_DEFINED__
#define __AIO4C_BUFFER_POOL_DEFINED__
typedef struct s_BufferPool BufferPool;
#endif /* __AIO4C_BUFFER_POOL_DEFINED__ */

/**
 * @fn Buffer* NewBuffer(int)
 * @brief Allocates a buffer.
 *
 * @param size
 *   The capacity of the allocated buffer.
 * @return
 *   A pointer to the allocated Buffer, or NULL if the requested capacity
 *   cannot be allocated for the buffer.
 */
extern AIO4C_API Buffer* NewBuffer(int size);

/**
 * @fn void FreeBuffer(Buffer**)
 * @brief Frees a previously allocated Buffer.
 *
 * If the Buffer is freed with success, the Buffer's pointer is set to NULL.
 *
 * @param pBuffer
 *   A pointer to a Buffer's pointer returned by NewBuffer(int). If the Buffer
 *   was allocated from a BufferPool, this method should not be used, and
 *   ReleaseBuffer MUST be used instead.
 */
extern AIO4C_API void FreeBuffer(Buffer** pBuffer);

/**
 * @fn BufferPool* NewBufferPool(aio4c_size_t)
 * @brief Creates a BufferPool.
 *
 * Initializes a BufferPool and creates a first batch of Buffers.
 *
 * @param bufferSize
 *   The capacity of allocated Buffers by this pool.
 * @return
 *   A pointer to the created BufferPool.
 *
 * @see AIO4C_BUFFER_POOL_BATCH_SIZE
 */
extern AIO4C_API BufferPool* NewBufferPool(aio4c_size_t bufferSize);

/**
 * @fn int GetBufferPoolBufferSize(BufferPool*)
 * @brief Retrieves the capacity of BufferPool's allocated Buffer.
 *
 * @param pool
 *   A pointer to the BufferPool to retrieve the Buffer's size from.
 * @return
 *   The capacity of the Buffers allocated by this BufferPool.
 */
extern AIO4C_API int GetBufferPoolBufferSize(BufferPool* pool);

/**
 * @fn Buffer* AllocateBuffer(BufferPool*)
 * @brief Allocates a Buffer from a BufferPool.
 *
 * If there is enough free Buffers in the pool, one of them is removed from the
 * pool and returned by this function. If the count of free Buffers is 0, then
 * AIO4C_BUFFER_POOL_BATCH_SIZE Buffers are allocated and one of them is
 * retrieved.
 *
 * Buffer's allocated using this function MUST be released using the function
 * ReleaseBuffer(Buffer).
 *
 * @param pool
 *   A pointer to a BufferPool.
 * @return
 *   A pointer to a free Buffer.
 */
extern AIO4C_API Buffer* AllocateBuffer(BufferPool* pool);

/**
 * @fn void ReleaseBuffer(Buffer**)
 * @brief Releases a Buffer to a BufferPool.
 *
 * Resets the Buffer and put it back into his BufferPool.
 *
 * @param pBuffer
 *   A pointer to a Buffer's pointer returned by AllocateBuffer(BufferPool).
 *   If the Buffer is released correctly, its pointer is then set to NULL.
 *
 * @see BufferReset(Buffer*)
 */
extern AIO4C_API void ReleaseBuffer(Buffer** pBuffer);

/**
 * @fn void FreeBufferPool(BufferPool**)
 * @brief Frees a BufferPool.
 *
 * Frees all Buffers of the pool, then the BufferPool. If everything goes fine,
 * sets the BufferPool pointer to NULL.
 *
 * @param pool
 *   A pointer to a BufferPool's pointer.
 *
 * @see FreeBuffer(Buffer**)
 */
extern AIO4C_API void FreeBufferPool(BufferPool** pool);

/**
 * @fn Buffer* BufferFlip(Buffer*)
 * @brief Flips a Buffer.
 *
 * The Buffer's limit is set to its current position, and its current position
 * is set to 0.
 *
 * @param buffer
 *   A pointer to the Buffer to flip.
 * @return
 *   The pointer to the flipped Buffer.
 */
extern AIO4C_API Buffer* BufferFlip(Buffer* buffer);

/**
 * @fn Buffer* BufferPosition(Buffer*,int)
 * @brief Sets the Buffer's current position.
 *
 * If the requested position exceeds the Buffer's limit, an error is logged
 * indicating that the position is invalid and NULL is returned.
 *
 * @param buffer
 *   A pointer to the Buffer to set the position for.
 * @param position
 *   The position to set.
 * @return
 *   The Buffer's pointer if the position was set correctly, or NULL if an
 *   error occurred.
 */
extern AIO4C_API Buffer* BufferPosition(Buffer* buffer, int position);

/**
 * @fn Buffer* BufferLimit(Buffer*,int)
 * @brief Sets the Buffer's limit.
 *
 * If the requested limit exceeds the Buffer's capacity, an error is logged
 * indicating that the limit is invalid and NULL is returned.
 *
 * @param buffer
 *   A pointer to the Buffer to set the limit for.
 * @param limit
 *   The limit to set.
 * @return
 *   The Buffer's pointer if the limit was set correctly, or NULL if an error
 *   occurred.
 */
extern AIO4C_API Buffer* BufferLimit(Buffer* buffer, int limit);

/**
 * @fn Buffer* BufferReset(Buffer*)
 * @brief Resets a Buffer.
 *
 * This function clear the Buffer's content by filling it with zeros, sets the
 * Buffer's position to 0 and the Buffer's limit to its capacity.
 *
 * @param buffer
 *   A pointer to the Buffer to reset.
 * @return
 *   The pointer to the resetted Buffer.
 */
extern AIO4C_API Buffer* BufferReset(Buffer* buffer);

/**
 * @fn Buffer* BufferCopy(Buffer*,Buffer*)
 * @brief Copy a Buffer.
 *
 * Copy a Buffer's data to another Buffer, taking into account the source
 * Buffer's position and limit, meaning that the recipient Buffer will hold
 * the source Buffer's data that is located between its current position and
 * limit.
 *
 * The recipient Buffer is resetted before performing the copy, and its
 * limit will be set to the amount of available data in the source Buffer.
 *
 * The source Buffer is left unmodified.
 *
 * @param dst
 *   A pointer to the Buffer that will receive the source Buffer's data.
 * @param src
 *   A pointer to the Buffer to copy data from.
 * @return
 *   A pointer to the copied Buffer.
 */
extern AIO4C_API Buffer* BufferCopy(Buffer* dst, Buffer* src);

/**
 * @fn int BufferGetPosition(Buffer*)
 * @brief Retrieves a Buffer's current position.
 *
 * @param buffer
 *   A pointer to the Buffer to retrieve the position from.
 * @return
 *   The Buffer's current position.
 */
extern AIO4C_API int BufferGetPosition(Buffer* buffer);

/**
 * @fn int BufferGetLimit(Buffer*)
 * @brief Retrieves a Buffer's current limit.
 *
 * @param buffer
 *   A pointer to the Buffer to retrieve the limit from.
 * @return
 *   The Buffer's current limit.
 */
extern AIO4C_API int BufferGetLimit(Buffer* buffer);

/**
 * @fn int BufferGetCapacity(Buffer*)
 * @brief Retrieves a Buffer's capacity.
 *
 * @param buffer
 *   A pointer to the Buffer to retrieve the capacity from.
 * @return
 *   The Buffer's capacity.
 */
extern AIO4C_API int BufferGetCapacity(Buffer* buffer);

/**
 * @fn int BufferRemaining(Buffer*)
 * @brief Retrieves the amount of available data in a Buffer.
 *
 * @param buffer
 *   A pointer to the Buffer to retrieve the amount of available data from.
 * @return
 *   The amount of available data in the Buffer, equals to the Buffer's
 *   position substracted from the Buffer's limit.
 */
extern AIO4C_API int BufferRemaining(Buffer* buffer);

/**
 * @fn bool BufferHasRemaining(Buffer*)
 * @brief Determines if a Buffer has remaining data.
 *
 * A Buffer is considered having remaining data when its position is less than
 * its limit.
 *
 * @param buffer
 *   A pointer to the Buffer to determine if it has remaining data.
 * @return
 *   true if the Buffer has remaining data, false in any other case.
 */
extern AIO4C_API bool BufferHasRemaining(Buffer* buffer);

/**
 * @fn aio4c_byte_t* BufferGetBytes(Buffer*)
 * @brief Retrieves a Buffer's byte array
 *
 * Operations to the returned byte array are mapped back to the Buffer.
 * Be aware that performing operations on the returned array will not update
 * the Buffer's position and limit, so you MUST sets thoses using related
 * functions.
 *
 * @param buffer
 *   A pointer to the Buffer to retrieve the byte array from.
 * @return
 *   A pointer to the Buffer's data.
 */
extern AIO4C_API aio4c_byte_t* BufferGetBytes(Buffer* buffer);

/**
 * @def BufferGetByte(buffer,out)
 * @brief Retrieves one byte from a Buffer.
 *
 * This macro is equivalent to the following:
 * @code
 *   BufferGet(buffer, out, sizeof(aio4c_byte_t))
 * @endcode
 *
 * The Buffer should have at least one byte of remaining data.
 *
 * @param buffer
 *   A pointer to the Buffer to retrieve a byte from.
 * @param out
 *   A pointer to a variable aio4c_byte_t to store the retrieved byte in.
 * @return
 *   true if the byte has been retrieved with success, false if there is not
 *   enough remaining data in the Buffer.
 *
 * @see BufferGet(Buffer*,void*,int)
 */
#define BufferGetByte(buffer,out) \
    BufferGet(buffer, out, sizeof(aio4c_byte_t))

/**
 * @def BufferGetShort(buffer,out)
 * @brief Retrieves one short from a Buffer.
 *
 * This macro is equivalent to the following:
 * @code
 *   BufferGet(buffer, out, sizeof(short))
 * @endcode
 *
 * The Buffer should have at least two bytes of remaining data.
 *
 * @param buffer
 *   A pointer to the Buffer to retrieve a short from.
 * @param out
 *   A pointer to a short variable to store the retrieved short in.
 * @return
 *   true if the short has been retrieved with success, false if there is not
 *   enough remaining data in the Buffer.
 *
 * @see BufferGet(Buffer*,void*,int)
 */
#define BufferGetShort(buffer,out) \
    BufferGet(buffer, out, sizeof(short))

/**
 * @def BufferGetInt(buffer,out)
 * @brief Retrieves one int from a Buffer.
 *
 * This macro is equivalent to the following:
 * @code
 *   BufferGet(buffer, out, sizeof(int))
 * @endcode
 *
 * The Buffer should have at least four bytes of remaining data.
 *
 * @param buffer
 *   A pointer to the Buffer to retrieve a int from.
 * @param out
 *   A pointer to an int variable to store the retrieved int in.
 * @return
 *   true if the int has been retrieved with success, false if there is not
 *   enough remaining data in the Buffer.
 *
 * @see BufferGet(Buffer*,void*,int)
 */
#define BufferGetInt(buffer,out) \
    BufferGet(buffer, out, sizeof(int))

/**
 * @def BufferGetLong(buffer,out)
 * @brief Retrieves one long from a Buffer.
 *
 * This macro is equivalent to the following:
 * @code
 *   BufferGet(buffer, out, sizeof(long))
 * @endcode
 *
 * The Buffer should have at least eight bytes of remaining data.
 *
 * @param buffer
 *   A pointer to the Buffer to retrieve a long from.
 * @param out
 *   A pointer to a long variable to store the retrieved long in.
 * @return
 *   true if the long has been retrieved with success, false if there is not
 *   enough remaining data in the Buffer.
 *
 * @see BufferGet(Buffer*,void*,int)
 */
#define BufferGetLong(buffer,out) \
    BufferGet(buffer, out, sizeof(long))

/**
 * @def BufferGetFloat(buffer,out)
 * @brief Retrieves one float from a Buffer.
 *
 * This macro is equivalent to the following:
 * @code
 *   BufferGet(buffer, out, sizeof(float))
 * @endcode
 *
 * The Buffer should have at least four bytes of remaining data.
 *
 * @param buffer
 *   A pointer to the Buffer to retrieve a float from.
 * @param out
 *   A pointer to a float variable to store the retrieved float in.
 * @return
 *   true if the float has been retrieved with success, false if there is not
 *   enough remaining data in the Buffer.
 *
 * @see BufferGet(Buffer*,void*,int)
 */
#define BufferGetFloat(buffer,out) \
    BufferGet(buffer, out, sizeof(float))

/**
 * @def BufferGetDouble(buffer,out)
 * @brief Retrieves one double from a Buffer.
 *
 * This macro is equivalent to the following:
 * @code
 *   BufferGet(buffer, out, sizeof(double))
 * @endcode
 *
 * The Buffer should have at least eight bytes of remaining data.
 *
 * @param buffer
 *   A pointer to the Buffer to retrieve a double from.
 * @param out
 *   A pointer to a double variable to store the retrieved double in.
 * @return
 *   true if the double has been retrieved with success, false if there is not
 *   enough remaining data in the Buffer.
 *
 * @see BufferGet(Buffer*,void*,int)
 */
#define BufferGetDouble(buffer,out) \
    BufferGet(buffer, out, sizeof(double))

/**
 * @fn bool BufferGet(Buffer*,void*,int)
 * @brief Retrieves data from a Buffer.
 *
 * Checks if the Buffer has enough remaining data to fulfill the request, if
 * there is enough, copy requested amount of data from the Buffer at its
 * current position in the pointed space, then increases the Buffer's current
 * position by the requested amount of data.
 *
 * If there is not enough remaining data in the Buffer, an
 * AIO4C_BUFFER_UNDERFLOW_ERROR will be raised.
 *
 * @param buffer
 *   A pointer to the Buffer to retrieve data from.
 * @param out
 *   A pointer to a memory zone where to put the retrieved data.
 * @param size
 *   The bytes count of data to retrieve from the Buffer.
 * @return
 *   true if there is enough remaining data in the Buffer, false if an
 *   underflow was detected (meaning that there was not enough remaining data).
 *
 * @see AIO4C_BUFFER_UNDERFLOW_ERROR
 */
extern AIO4C_API bool BufferGet(Buffer* buffer, void* out, int size);

/**
 * @fn bool BufferGetString(Buffer*,char**)
 * @brief Retrieves a string from a Buffer.
 *
 * Tries to retrieve a null-terminated string from the Buffer.
 * The returned string will start at the Buffer's current position, and:
 *   - if a null character (\\0) is found in the Buffer, the Buffer's current
 *   position will be increased by the length of the returned string plus one,
 *   - if no null character (\\0) is found between the current Buffer's position
 *   and its limit, then it is added at the Buffer's limit, and its position is
 *   set to its limit. In that case, the returned string length will be equals
 *   to the amount of remaining data in the Buffer.
 *   - if there is no remaining data, then the returned string will be an empty
 *   string.
 *
 * @param buffer
 *   The Buffer to retrieve the string from.
 * @param out
 *   A pointer where to store the returned string's pointer.
 * @return
 *   Always true.
 */
extern AIO4C_API bool BufferGetString(Buffer* buffer, char** out);

/**
 * @def BufferPutByte(buffer,in)
 * @brief Stores a byte in a Buffer.
 *
 * This macro is equivalent to the following:
 * @code
 *   BufferPut(buffer, in, sizeof(aio4c_byte_t))
 * @endcode
 *
 * The buffer should have at least one byte of remaining space.
 *
 * @param buffer
 *   A pointer to the Buffer where to store the byte.
 * @param in
 *   A pointer to a byte variable.
 * @return
 *   true if the byte as been stored into the Buffer, false if an overflow has
 *   been detected (meaning that there was not enough remaining space in the
 *   Buffer).
 *
 * @see BufferPut(Buffer*,void*,int)
 */
#define BufferPutByte(buffer, in) \
    BufferPut(buffer, in, sizeof(aio4c_byte_t))

/**
 * @def BufferPutShort(buffer,in)
 * @brief Stores a short in a Buffer.
 *
 * This macro is equivalent to the following:
 * @code
 *   BufferPut(buffer, in, sizeof(short))
 * @endcode
 *
 * The buffer should have at least two bytes of remaining space.
 *
 * @param buffer
 *   A pointer to the Buffer where to store the short.
 * @param in
 *   A pointer to a short variable.
 * @return
 *   true if the short as been stored into the Buffer, false if an overflow has
 *   been detected (meaning that there was not enough remaining space in the
 *   Buffer).
 *
 * @see BufferPut(Buffer*,void*,int)
 */
#define BufferPutShort(buffer,in) \
    BufferPut(buffer, in, sizeof(short))

/**
 * @def BufferPutInt(buffer,in)
 * @brief Stores an int in a Buffer.
 *
 * This macro is equivalent to the following:
 * @code
 *   BufferPut(buffer, in, sizeof(int))
 * @endcode
 *
 * The buffer should have at least four bytes of remaining space.
 *
 * @param buffer
 *   A pointer to the Buffer where to store the int.
 * @param in
 *   A pointer to an int variable.
 * @return
 *   true if the int as been stored into the Buffer, false if an overflow has
 *   been detected (meaning that there was not enough remaining space in the
 *   Buffer).
 *
 * @see BufferPut(Buffer*,void*,int)
 */
#define BufferPutInt(buffer,in) \
    BufferPut(buffer, in, sizeof(int))

/**
 * @def BufferPutLong(buffer,in)
 * @brief Stores a long in a Buffer.
 *
 * This macro is equivalent to the following:
 * @code
 *   BufferPut(buffer, in, sizeof(long))
 * @endcode
 *
 * The buffer should have at least eight bytes of remaining space.
 *
 * @param buffer
 *   A pointer to the Buffer where to store the long.
 * @param in
 *   A pointer to a long variable.
 * @return
 *   true if the long as been stored into the Buffer, false if an overflow has
 *   been detected (meaning that there was not enough remaining space in the
 *   Buffer).
 *
 * @see BufferPut(Buffer*,void*,int)
 */
#define BufferPutLong(buffer,in) \
    BufferPut(buffer, in, sizeof(long))

/**
 * @def BufferPutFloat(buffer,in)
 * @brief Stores a float in a Buffer.
 *
 * This macro is equivalent to the following:
 * @code
 *   BufferPut(buffer, in, sizeof(float))
 * @endcode
 *
 * The buffer should have at least four bytes of remaining space.
 *
 * @param buffer
 *   A pointer to the Buffer where to store the float.
 * @param in
 *   A pointer to a float variable.
 * @return
 *   true if the float as been stored into the Buffer, false if an overflow has
 *   been detected (meaning that there was not enough remaining space in the
 *   Buffer).
 *
 * @see BufferPut(Buffer*,void*,int)
 */
#define BufferPutFloat(buffer,in) \
    BufferPut(buffer, in, sizeof(float))

/**
 * @def BufferPutDouble(buffer,in)
 * @brief Stores a double in a Buffer.
 *
 * This macro is equivalent to the following:
 * @code
 *   BufferPut(buffer, in, sizeof(double))
 * @endcode
 *
 * The buffer should have at least eight bytes of remaining space.
 *
 * @param buffer
 *   A pointer to the Buffer where to store the double.
 * @param in
 *   A pointer to a double variable.
 * @return
 *   true if the double as been stored into the Buffer, false if an overflow
 *   has been detected (meaning that there was not enough remaining space in
 *   the Buffer).
 *
 * @see BufferPut(Buffer*,void*,int)
 */
#define BufferPutDouble(buffer,in) \
    BufferPut(buffer, in, sizeof(double))

/**
 * @fn BufferPutString(Buffer*,char*)
 * @brief Stores a string in a Buffer.
 *
 * The buffer should have at least strlen(in) plus one bytes of remaining
 * space.
 *
 * @param buffer
 *   A pointer to the Buffer where to store the string.
 * @param in
 *   The string to put in the Buffer.
 * @return
 *   true if the string as been stored into the Buffer, false if an overflow has
 *   been detected (meaning that there was not enough remaining space in the
 *   Buffer).
 */
extern AIO4C_API bool BufferPutString(Buffer* buffer, char* in);

/**
 * @fn bool BufferPut(Buffer*,void*,int)
 * @brief Stores data in a Buffer.
 *
 * Checks that there is enough space in the Buffer to store the requested
 * amount of data, if this is the case, then copy the requested amount of data
 * starting at the Buffer's position, and increase the Buffer's current
 * position by the amount of stored data.
 *
 * If there is not enough space in the Buffer, then an
 * AIO4C_BUFFER_OVERFLOW_ERROR will be raised.
 *
 * @param buffer
 *   A pointer to the Buffer to store the data in.
 * @param in
 *   A pointer to the data to store into the Buffer.
 * @param size
 *   The size of the data to store into the Buffer.
 * @return
 *   true if the data has been stored in the Buffer, false if an overflow was
 *   detected (meaning that there was not enough remaining space in the
 *   Buffer).
 *
 * @see AIO4C_BUFFER_OVERFLOW_ERROR
 */
extern AIO4C_API bool BufferPut(Buffer* buffer, void* in, int size);

#endif /* __AIO4C_BUFFER_H__ */
