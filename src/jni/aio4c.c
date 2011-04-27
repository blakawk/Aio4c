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
#include <aio4c/jni/aio4c.h>

#include <aio4c.h>
#include <aio4c/alloc.h>

#include <string.h>

static int _argc = 0;
static char** _argv = NULL;

JNIEXPORT void JNICALL Java_com_aio4c_Aio4c_usage(JNIEnv* jvm __attribute__((unused)), jclass aio4c __attribute__((unused))) {
    Aio4cUsage();
}

JNIEXPORT void JNICALL Java_com_aio4c_Aio4c_init(JNIEnv* jvm, jclass aio4c __attribute__((unused)), jobjectArray args) {
    jsize nbArgs = (*jvm)->GetArrayLength(jvm, args);
    _argc = nbArgs + 1;
    _argv = aio4c_malloc(_argc * sizeof(char*));
    char* arg = NULL;
    jstring jarg = NULL;
    jsize i = 0;

    _argv[0] = "aio4c_jni";

    for (i = 0; i < nbArgs; i++) {
        jarg = (jstring)(*jvm)->GetObjectArrayElement(jvm, args, i);
        arg = (char*)(*jvm)->GetStringUTFChars(jvm, jarg, NULL);
        _argv[i + 1] = aio4c_malloc((strlen(arg) + 1));
        memcpy(_argv[i + 1], arg, strlen(arg) + 1);
        (*jvm)->ReleaseStringUTFChars(jvm, jarg, arg);
    }

    Aio4cInit(_argc, _argv);
}

JNIEXPORT void JNICALL Java_com_aio4c_Aio4c_end(JNIEnv* jvm __attribute__((unused)), jclass aio4c __attribute__((unused))) {
    int i = 0;

    Aio4cEnd();

    if (_argv != NULL) {
        for (i = 1; i < _argc; i++) {
            if (_argv[i] != NULL) {
                aio4c_free(_argv[i]);
                _argv[i] = NULL;
            }
        }
        aio4c_free(_argv);
        _argv = NULL;
    }
}
