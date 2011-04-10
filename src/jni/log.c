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
#include <aio4c/jni/log.h>

#include <aio4c/jni.h>
#include <aio4c/log.h>

JNIEXPORT void JNICALL Java_com_aio4c_Log_log(JNIEnv* jvm, jclass log __attribute__((unused)), jobject level, jstring message) {
    int _level = 0;
    char* _message = NULL;

    GetField(jvm, level, "value", "I", &_level);

    _message = (char*)(*jvm)->GetStringUTFChars(jvm, message, NULL);

    Log(_level, "%s", _message);

    (*jvm)->ReleaseStringUTFChars(jvm, message, _message);
}

