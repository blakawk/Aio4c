/**
 * This file is part of Aio4c <http://aio4c.so>.
 *
 * Aio4c <http://aio4c.so> is free software: you
 * can  redistribute  it  and/or modify it under
 * the  terms  of the GNU General Public License
 * as published by the Free Software Foundation,
 * version 3 of the License.
 *
 * This  program is distributed in the hope that
 * it  will be useful, but WITHOUT ANY WARRANTY;
 * without   even   the   implied   warranty  of
 * MERCHANTABILITY  or  FITNESS FOR A PARTICULAR
 * PURPOSE.
 *
 * See  the  GNU General Public License for more
 * details.  You  should have received a copy of
 * the  GNU  General  Public  License along with
 * Aio4c    <http://aio4c.so>.   If   not,   see
 * <http://www.gnu.org/licenses/>.
 */
#ifndef __AIO4C_JNI_H__
#define __AIO4C_JNI_H__

#include <aio4c/buffer.h>

#include <jni.h>
#include <stdarg.h>

#define CheckJNICall(jvm,call,result) \
    _CheckJNICall(__FILE__, __LINE__, jvm, (void*)call, #call, (void**)&result)
extern AIO4C_API void _CheckJNICall(char* file, int line, JNIEnv* jvm, void* result, char* call, void** stock);

extern AIO4C_API void RaiseException(JNIEnv* jvm, char* name, char* signature, ...);

extern AIO4C_API void GetPointer(JNIEnv* jvm, jobject object, void** pointer);

extern AIO4C_API void SetPointer(JNIEnv* jvm, jobject object, void* pointer);

extern AIO4C_API void GetField(JNIEnv* jvm, jobject object, char* name, char* signature, void* out);

extern AIO4C_API void SetField(JNIEnv* jvm, jobject object, char* name, char* signature, void* out);

extern AIO4C_API jobject New_com_aio4c_Buffer(JNIEnv* jvm, Buffer* buffer);

#endif
