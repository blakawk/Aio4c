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
#include <aio4c/jni/aio4c.h>

#include <aio4c.h>

JNIEXPORT void JNICALL Java_com_aio4c_Aio4c_init(JNIEnv* jvm, jclass aio4c __attribute__((unused)), jobject level, jstring log) {
    int _level = 0;
    char* _log = NULL;
    jclass cLogLevel;
    jfieldID fLogLevel_value = NULL;

    cLogLevel = (*jvm)->GetObjectClass(jvm, level);
    fLogLevel_value = (*jvm)->GetFieldID(jvm, cLogLevel, "value", "I");
    _level = (int)(*jvm)->GetIntField(jvm, level, fLogLevel_value);

    _log = (char*)(*jvm)->GetStringUTFChars(jvm, log, NULL);

    Aio4cInit(_level, _log);

    (*jvm)->ReleaseStringUTFChars(jvm, log, _log);
}

JNIEXPORT void JNICALL Java_com_aio4c_Aio4c_end(JNIEnv* jvm __attribute__((unused)), jclass aio4c __attribute__((unused))) {
    Aio4cEnd();
}
