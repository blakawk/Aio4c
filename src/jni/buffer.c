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
#include <aio4c/jni/buffer.h>

#include <aio4c/alloc.h>
#include <aio4c/jni.h>
#include <aio4c/buffer.h>

#define _ThrowBufferUnderflow(jvm,buffer) \
    RaiseException(jvm,"com/aio4c/buffer/BufferUnderflowException","(Lcom/aio4c/buffer/Buffer;)V",buffer)

#define _ThrowBufferOverflow(jvm,buffer) \
    RaiseException(jvm,"com/aio4c/buffer/BufferOverflowException","(Lcom/aio4c/buffer/Buffer;)V",buffer)

static Buffer* _GetBuffer(JNIEnv* jvm, jobject buffer) {
    Buffer* _buffer = NULL;

    GetPointer(jvm, buffer, (void**)&_buffer);

    return _buffer;
}

JNIEXPORT jobject JNICALL Java_com_aio4c_buffer_Buffer_allocate(JNIEnv* jvm, jclass clazz, jint capacity) {
    Buffer* buffer = NULL;
    jmethodID constructor = NULL;
    jobject jBuffer = NULL;
    jstring jmsg = NULL;

    CheckJNICall(jvm, (*jvm)->GetMethodID(jvm, clazz, "<init>", "()V"), constructor);
    CheckJNICall(jvm, (*jvm)->NewObject(jvm, clazz, constructor), jBuffer);

    buffer = NewBuffer(capacity);

    if (buffer == NULL) {
        CheckJNICall(jvm, (*jvm)->NewStringUTF(jvm, "cannot allocate buffer"), jmsg);
        RaiseException(
                jvm,
                "java/lang/OutOfMemoryError",
                "(Ljava/lang/String;)V",
                jmsg);
    } else {
        SetPointer(jvm, jBuffer, buffer);
    }

    return jBuffer;
}

JNIEXPORT jbyteArray JNICALL Java_com_aio4c_buffer_Buffer_array__(JNIEnv* jvm, jobject buffer) {
    Buffer* _buffer = NULL;
    jbyteArray array = NULL;

    _buffer = _GetBuffer(jvm, buffer);

    array = (*jvm)->NewByteArray(jvm, _buffer->limit - _buffer->position);
    (*jvm)->SetByteArrayRegion(jvm, array, 0, _buffer->limit - _buffer->position, (jbyte*)&_buffer->data[_buffer->position]);

    return array;
}

JNIEXPORT void JNICALL Java_com_aio4c_buffer_Buffer_array___3B(JNIEnv* jvm, jobject buffer, jbyteArray array) {
    Buffer* _buffer = _GetBuffer(jvm, buffer);

    (*jvm)->GetByteArrayRegion(jvm, array, 0, _buffer->limit - _buffer->position, (jbyte*)&_buffer->data[_buffer->position]);
}

JNIEXPORT jbyte JNICALL Java_com_aio4c_buffer_Buffer_get(JNIEnv* jvm, jobject buffer) {
    Buffer* _buffer = _GetBuffer(jvm, buffer);
    jbyte _byte = 0;

    if (!BufferGetByte(_buffer, &_byte)) {
        _ThrowBufferUnderflow(jvm, buffer);
    }

    return _byte;
}

JNIEXPORT jshort JNICALL Java_com_aio4c_buffer_Buffer_getShort(JNIEnv* jvm, jobject buffer) {
    Buffer* _buffer = _GetBuffer(jvm, buffer);
    jshort _short = 0;

    if (!BufferGetShort(_buffer, &_short)) {
        _ThrowBufferUnderflow(jvm, buffer);
    }

    return _short;
}

JNIEXPORT jint JNICALL Java_com_aio4c_buffer_Buffer_getInt(JNIEnv* jvm, jobject buffer) {
    Buffer* _buffer = _GetBuffer(jvm, buffer);
    jint _int = 0;

    if (!BufferGetInt(_buffer, &_int)) {
        _ThrowBufferUnderflow(jvm, buffer);
    }

    return _int;
}

JNIEXPORT jlong JNICALL Java_com_aio4c_buffer_Buffer_getLong(JNIEnv* jvm, jobject buffer) {
    Buffer* _buffer = _GetBuffer(jvm, buffer);
    jlong _long = 0;

    if (!BufferGet(_buffer, &_long, sizeof(jlong))) {
        _ThrowBufferUnderflow(jvm, buffer);
    }

    return _long;
}

JNIEXPORT jfloat JNICALL Java_com_aio4c_buffer_Buffer_getFloat(JNIEnv* jvm, jobject buffer) {
    Buffer* _buffer = _GetBuffer(jvm, buffer);
    jfloat _float = 0;

    if (!BufferGetFloat(_buffer, &_float)) {
        _ThrowBufferUnderflow(jvm, buffer);
    }

    return _float;
}

JNIEXPORT jdouble JNICALL Java_com_aio4c_buffer_Buffer_getDouble(JNIEnv* jvm, jobject buffer) {
    Buffer* _buffer = _GetBuffer(jvm, buffer);
    jdouble _double = 0;

    if (!BufferGetDouble(_buffer, &_double)) {
        _ThrowBufferUnderflow(jvm, buffer);
    }

    return _double;
}

JNIEXPORT jstring JNICALL Java_com_aio4c_buffer_Buffer_getString(JNIEnv* jvm, jobject buffer) {
    Buffer* _buffer = _GetBuffer(jvm, buffer);
    char* str = NULL;
    jstring _string = NULL;

    BufferGetString(_buffer, &str);

    _string = (*jvm)->NewStringUTF(jvm, str);

    return _string;
}

JNIEXPORT void JNICALL Java_com_aio4c_buffer_Buffer_put(JNIEnv* jvm, jobject buffer, jbyte b) {
    Buffer* _buffer = _GetBuffer(jvm, buffer);

    if (!BufferPutByte(_buffer, &b)) {
        _ThrowBufferOverflow(jvm, buffer);
    }
}

JNIEXPORT void JNICALL Java_com_aio4c_buffer_Buffer_putShort(JNIEnv* jvm, jobject buffer, jshort s) {
    Buffer* _buffer = _GetBuffer(jvm, buffer);

    if (!BufferPutShort(_buffer, &s)) {
        _ThrowBufferOverflow(jvm, buffer);
    }
}

JNIEXPORT void JNICALL Java_com_aio4c_buffer_Buffer_putInt(JNIEnv* jvm, jobject buffer, jint i) {
    Buffer* _buffer = _GetBuffer(jvm, buffer);

    if (!BufferPutInt(_buffer, &i)) {
        _ThrowBufferOverflow(jvm, buffer);
    }
}

JNIEXPORT void JNICALL Java_com_aio4c_buffer_Buffer_putLong(JNIEnv* jvm, jobject buffer, jlong l) {
    Buffer* _buffer = _GetBuffer(jvm, buffer);

    if (!BufferPut(_buffer, &l, sizeof(jlong))) {
        _ThrowBufferOverflow(jvm, buffer);
    }
}

JNIEXPORT void JNICALL Java_com_aio4c_buffer_Buffer_putFloat(JNIEnv* jvm, jobject buffer, jfloat f) {
    Buffer* _buffer = _GetBuffer(jvm, buffer);

    if (!BufferPutFloat(_buffer, &f)) {
        _ThrowBufferOverflow(jvm, buffer);
    }
}

JNIEXPORT void JNICALL Java_com_aio4c_buffer_Buffer_putDouble(JNIEnv* jvm, jobject buffer, jdouble d) {
    Buffer* _buffer = _GetBuffer(jvm, buffer);

    if (!BufferPutDouble(_buffer, &d)) {
        _ThrowBufferOverflow(jvm, buffer);
    }
}

JNIEXPORT void JNICALL Java_com_aio4c_buffer_Buffer_putString(JNIEnv* jvm, jobject buffer, jstring s) {
    Buffer* _buffer = _GetBuffer(jvm, buffer);
    const char* str = (*jvm)->GetStringUTFChars(jvm, s, NULL);

    if (!BufferPutString(_buffer, (char*)str)) {
        _ThrowBufferOverflow(jvm, buffer);
    }

    (*jvm)->ReleaseStringUTFChars(jvm, s, str);
}

JNIEXPORT jboolean JNICALL Java_com_aio4c_buffer_Buffer_hasRemaining(JNIEnv* jvm, jobject buffer) {
    Buffer* _buffer = _GetBuffer(jvm, buffer);

    return BufferHasRemaining(_buffer);
}

JNIEXPORT jint JNICALL Java_com_aio4c_buffer_Buffer_position__(JNIEnv* jvm, jobject buffer) {
    Buffer* _buffer = _GetBuffer(jvm, buffer);

    return _buffer->position;
}

JNIEXPORT void JNICALL Java_com_aio4c_buffer_Buffer_position__I(JNIEnv* jvm, jobject buffer, jint position) {
    Buffer* _buffer = _GetBuffer(jvm, buffer);

    if (BufferPosition(_buffer, position) == NULL) {
        (*jvm)->ThrowNew(jvm, (*jvm)->FindClass(jvm, "Ljava/lang/IllegalArgumentException;"), "invalid position");
        return;
    }
}

JNIEXPORT jint JNICALL Java_com_aio4c_buffer_Buffer_limit__(JNIEnv* jvm, jobject buffer) {
    Buffer* _buffer = _GetBuffer(jvm, buffer);

    return _buffer->limit;
}

JNIEXPORT void JNICALL Java_com_aio4c_buffer_Buffer_limit__I(JNIEnv* jvm, jobject buffer, jint limit) {
    Buffer* _buffer = _GetBuffer(jvm, buffer);

    if (BufferLimit(_buffer, limit) == NULL) {
        (*jvm)->ThrowNew(jvm, (*jvm)->FindClass(jvm, "Ljava/lang/IllegalArgumentException;"), "invalid limit");
        return;
    }
}

JNIEXPORT jint JNICALL Java_com_aio4c_buffer_Buffer_capacity(JNIEnv* jvm, jobject buffer) {
    Buffer* _buffer = _GetBuffer(jvm, buffer);

    return _buffer->size;
}

JNIEXPORT void JNICALL Java_com_aio4c_buffer_Buffer_flip(JNIEnv* jvm, jobject buffer) {
    Buffer* _buffer = _GetBuffer(jvm, buffer);

    BufferFlip(_buffer);
}

JNIEXPORT void JNICALL Java_com_aio4c_buffer_Buffer_reset(JNIEnv* jvm, jobject buffer) {
    Buffer* _buffer = _GetBuffer(jvm, buffer);

    BufferReset(_buffer);
}

JNIEXPORT jstring JNICALL Java_com_aio4c_buffer_Buffer_toString(JNIEnv* jvm, jobject buffer) {
    Buffer* _buffer = _GetBuffer(jvm, buffer);
#ifndef AIO4C_WIN32
    int len = snprintf(NULL, 0, "buffer %p [cap: %d, lim: %d, pos: %d]", (void*)_buffer, _buffer->size, _buffer->limit, _buffer->position);
    char* _s = aio4c_malloc(len + 1);
    snprintf(_s, len + 1, "buffer %p [cap: %d, lim: %d, pos: %d]", (void*)_buffer, _buffer->size, _buffer->limit, _buffer->position);
#else /* AIO4C_WIN32 */
    int len = strlen("buffer %p [cap: %d, lim: %d, pos: %d]") + 1;
    char* _s = aio4c_malloc(len);
    while (_s != NULL && snprintf(_s, len, "buffer %p [cap: %d, lim: %d, pos: %d]", (void*)_buffer, _buffer->size, _buffer->limit, _buffer->position) < 0) {
        len++;
        _s = aio4c_realloc(_s, len);
    }
#endif /* AIO4C_WIN32 */
    jstring s = (*jvm)->NewStringUTF(jvm, _s);
    aio4c_free(_s);

    return s;
}
