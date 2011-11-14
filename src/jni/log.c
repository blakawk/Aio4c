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
#include <aio4c/jni.h>
#include <aio4c/log.h>

#include "com_aio4c_log_Log.h"

JNIEXPORT void JNICALL Java_com_aio4c_log_Log_log(JNIEnv* jvm, jclass log __attribute__((unused)), jobject level, jstring message) {
    int _level = 0;
    char* _message = NULL;

    GetField(jvm, level, "value", "I", &_level);

    _message = (char*)(*jvm)->GetStringUTFChars(jvm, message, NULL);

    Log(_level, "%s", _message);

    (*jvm)->ReleaseStringUTFChars(jvm, message, _message);
}

