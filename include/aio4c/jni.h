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
#ifndef __AIO4C_JNI_H__
#define __AIO4C_JNI_H__

#include <aio4c/buffer.h>

#include <jni.h>

#define CheckJNICall(jvm,call,result) \
    _CheckJNICall(__FILE__, __LINE__, jvm, (void*)call, #call, (void**)&result)
extern void _CheckJNICall(char* file, int line, JNIEnv* jvm, void* result, char* call, void** stock);

extern void GetPointer(JNIEnv* jvm, jobject object, void** pointer);

extern void SetPointer(JNIEnv* jvm, jobject object, void* pointer);

extern void GetField(JNIEnv* jvm, jobject object, char* name, char* signature, void* out);

extern void SetField(JNIEnv* jvm, jobject object, char* name, char* signature, void* out);

extern jobject New_com_aio4c_Buffer(JNIEnv* jvm, Buffer* buffer);

#endif
