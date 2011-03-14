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
#include <aio4c/jni.h>

#include <aio4c/error.h>

#include <string.h>

void GetPointer(JNIEnv* jvm, jobject object, void** pointer) {
#if   AIO4C_P_SIZE == 4
    jint iPointer = 0;
    GetField(jvm, object, "iPointer", "I", &iPointer);
    *pointer = (void*)iPointer;
#elif AIO4C_P_SIZE == 8
    jlong lPointer = 0;
    GetField(jvm, object, "lPointer", "J", &lPointer);
    *pointer = (void*)lPointer;
#endif
}

void SetPointer(JNIEnv* jvm, jobject object, void* pointer) {
#if   AIO4C_P_SIZE == 4
    jint iPointer = (jint)pointer;
    SetField(jvm, object, "iPointer", "I", &iPointer);
#elif AIO4C_P_SIZE == 8
    jlong lPointer = (jlong)pointer;
    SetField(jvm, object, "lPointer", "J", &lPointer);
#endif
}

static jfieldID _FindField(JNIEnv* jvm, jobject object, char* name, char* signature) {
    ErrorCode code = AIO4C_ERROR_CODE_INITIALIZER;
    jclass clazz = NULL;
    jfieldID field = NULL;

    clazz = (*jvm)->GetObjectClass(jvm, object);
    field = (*jvm)->GetFieldID(jvm, clazz, name, signature);

    if (field == NULL) {
        code.field = name;
        code.signature = signature;
        Raise(AIO4C_LOG_LEVEL_ERROR, AIO4C_JNI_ERROR_TYPE, AIO4C_JNI_FIELD_ERROR, &code);
        return NULL;
    }

    return field;
}

void GetField(JNIEnv* jvm, jobject object, char* name, char* signature, void* out) {
    jfieldID field = _FindField(jvm, object, name, signature);

    if (field == NULL) {
        return;
    }

    switch (signature[0]) {
        case 'B':
            *(jbyte*)out = (*jvm)->GetByteField(jvm, object, field);
            break;
        case 'S':
            *(jshort*)out = (*jvm)->GetShortField(jvm, object, field);
            break;
        case 'I':
            *(jint*)out = (*jvm)->GetIntField(jvm, object, field);
            break;
        case 'J':
            *(jlong*)out = (*jvm)->GetLongField(jvm, object, field);
            break;
        case 'F':
            *(jfloat*)out = (*jvm)->GetFloatField(jvm, object, field);
            break;
        case 'D':
            *(jdouble*)out = (*jvm)->GetDoubleField(jvm, object, field);
            break;
        case 'L':
        case '[':
            *(jobject*)out = (*jvm)->GetObjectField(jvm, object, field);
            break;
        default:
            break;
    }
}

void SetField(JNIEnv* jvm, jobject object, char* name, char* signature, void* in) {
    jfieldID field = _FindField(jvm, object, name, signature);

    if (field == NULL) {
        return;
    }

    switch (signature[0]) {
        case 'B':
            (*jvm)->SetByteField(jvm, object, field, *(jbyte*)in);
            break;
        case 'S':
            (*jvm)->SetShortField(jvm, object, field, *(jshort*)in);
            break;
        case 'I':
            (*jvm)->SetIntField(jvm, object, field, *(jint*)in);
            break;
        case 'J':
            (*jvm)->SetLongField(jvm, object, field, *(jlong*)in);
            break;
        case 'F':
            (*jvm)->SetFloatField(jvm, object, field, *(jfloat*)in);
            break;
        case 'D':
            (*jvm)->SetDoubleField(jvm, object, field, *(jdouble*)in);
            break;
        case 'L':
        case '[':
            (*jvm)->SetObjectField(jvm, object, field, *(jobject*)in);
            break;
        default:
            break;
    }
}

jobject New_com_aio4c_Buffer(JNIEnv* jvm, Buffer* buffer) {
    jclass cBuffer = NULL;
    jmethodID mBuffer = NULL;
    jobject com_aio4c_Buffer = NULL;

    cBuffer = (*jvm)->FindClass(jvm, "com/aio4c/Buffer");
    mBuffer = (*jvm)->GetMethodID(jvm, cBuffer, "<init>", "()V");
    com_aio4c_Buffer = (*jvm)->NewObject(jvm, cBuffer, mBuffer);
    SetPointer(jvm, com_aio4c_Buffer, buffer);
    return com_aio4c_Buffer;
}
