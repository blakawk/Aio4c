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
#include <aio4c/jni/buffer.h>

#include <aio4c/jni.h>
#include <aio4c/buffer.h>

static Buffer* _GetBuffer(JNIEnv* jvm, jobject buffer) {
    Buffer* _buffer = NULL;

    GetPointer(jvm, buffer, (void**)&_buffer);

    return _buffer;
}

JNIEXPORT jbyteArray JNICALL Java_com_aio4c_Buffer_array(JNIEnv* jvm, jobject buffer) {
    Buffer* _buffer = NULL;
    jbyteArray array = NULL;

    _buffer = _GetBuffer(jvm, buffer);

    array = (*jvm)->NewByteArray(jvm, _buffer->limit - _buffer->position);
    (*jvm)->SetByteArrayRegion(jvm, array, 0, _buffer->limit - _buffer->position, (jbyte*)&_buffer->data[_buffer->position]);

    return array;
}

JNIEXPORT jbyte JNICALL Java_com_aio4c_Buffer_get(JNIEnv* jvm, jobject buffer) {
    Buffer* _buffer = _GetBuffer(jvm, buffer);
    jbyte _byte = 0;

    BufferGetByte(_buffer, &_byte);

    return _byte;
}

JNIEXPORT jshort JNICALL Java_com_aio4c_Buffer_getShort(JNIEnv* jvm, jobject buffer) {
    Buffer* _buffer = _GetBuffer(jvm, buffer);
    jshort _short = 0;

    BufferGetShort(_buffer, &_short);

    return _short;
}

JNIEXPORT jint JNICALL Java_com_aio4c_Buffer_getInt(JNIEnv* jvm, jobject buffer) {
    Buffer* _buffer = _GetBuffer(jvm, buffer);
    jint _int = 0;

    BufferGetInt(_buffer, &_int);

    return _int;
}

JNIEXPORT jlong JNICALL Java_com_aio4c_Buffer_getLong(JNIEnv* jvm, jobject buffer) {
    Buffer* _buffer = _GetBuffer(jvm, buffer);
    jlong _long = 0;

    BufferGet(_buffer, &_long, sizeof(jlong));

    return _long;
}

JNIEXPORT jfloat JNICALL Java_com_aio4c_Buffer_getFloat(JNIEnv* jvm, jobject buffer) {
    Buffer* _buffer = _GetBuffer(jvm, buffer);
    jfloat _float = 0;

    BufferGetFloat(_buffer, &_float);

    return _float;
}

JNIEXPORT jdouble JNICALL Java_com_aio4c_Buffer_getDouble(JNIEnv* jvm, jobject buffer) {
    Buffer* _buffer = _GetBuffer(jvm, buffer);
    jdouble _double = 0;

    BufferGetDouble(_buffer, &_double);

    return _double;
}

JNIEXPORT jstring JNICALL Java_com_aio4c_Buffer_getString(JNIEnv* jvm, jobject buffer) {
    Buffer* _buffer = _GetBuffer(jvm, buffer);
    char* str = NULL;
    jstring _string = NULL;

    BufferGetString(_buffer, &str);

    _string = (*jvm)->NewStringUTF(jvm, str);

    return _string;
}

JNIEXPORT void JNICALL Java_com_aio4c_Buffer_put(JNIEnv* jvm, jobject buffer, jbyte b) {
    Buffer* _buffer = _GetBuffer(jvm, buffer);

    BufferPutByte(_buffer, &b);
}

JNIEXPORT void JNICALL Java_com_aio4c_Buffer_putShort(JNIEnv* jvm, jobject buffer, jshort s) {
    Buffer* _buffer = _GetBuffer(jvm, buffer);

    BufferPutShort(_buffer, &s);
}

JNIEXPORT void JNICALL Java_com_aio4c_Buffer_putInt(JNIEnv* jvm, jobject buffer, jint i) {
    Buffer* _buffer = _GetBuffer(jvm, buffer);

    BufferPutInt(_buffer, &i);
}

JNIEXPORT void JNICALL Java_com_aio4c_Buffer_putLong(JNIEnv* jvm, jobject buffer, jlong l) {
    Buffer* _buffer = _GetBuffer(jvm, buffer);

    BufferPut(_buffer, &l, sizeof(jlong));
}

JNIEXPORT void JNICALL Java_com_aio4c_Buffer_putFloat(JNIEnv* jvm, jobject buffer, jfloat f) {
    Buffer* _buffer = _GetBuffer(jvm, buffer);

    BufferPutFloat(_buffer, &f);
}

JNIEXPORT void JNICALL Java_com_aio4c_Buffer_putDouble(JNIEnv* jvm, jobject buffer, jdouble d) {
    Buffer* _buffer = _GetBuffer(jvm, buffer);

    BufferPutDouble(_buffer, &d);
}

JNIEXPORT void JNICALL Java_com_aio4c_Buffer_putString(JNIEnv* jvm, jobject buffer, jstring s) {
    Buffer* _buffer = _GetBuffer(jvm, buffer);
    const char* str = (*jvm)->GetStringUTFChars(jvm, s, NULL);

    BufferPutString(_buffer, (char*)str);

    (*jvm)->ReleaseStringUTFChars(jvm, s, str);
}
