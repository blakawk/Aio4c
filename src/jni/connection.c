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
#include <aio4c/connection.h>
#include <aio4c/jni.h>

#include "com_aio4c_Connection.h"

JNIEXPORT void JNICALL Java_com_aio4c_Connection_enableWriteInterest(JNIEnv* jvm, jobject connection) {
    void* myConnection = NULL;

    GetPointer(jvm, connection, &myConnection);

    EnableWriteInterest(myConnection);
}

JNIEXPORT jstring JNICALL Java_com_aio4c_Connection_toString(JNIEnv* jvm, jobject connection) {
    void* pConnection = NULL;
    Connection* myConnection = NULL;
    jstring string = NULL;

    GetPointer(jvm, connection, &pConnection);
    myConnection = (Connection*)pConnection;

    string = (*jvm)->NewStringUTF(jvm, myConnection->string);

    return string;
}

JNIEXPORT void JNICALL Java_com_aio4c_Connection_close(JNIEnv* jvm, jobject connection, jboolean force) {
    void* myConnection = NULL;

    GetPointer(jvm, connection, &myConnection);

    ConnectionClose(myConnection, force);
}

JNIEXPORT jboolean JNICALL Java_com_aio4c_Connection_closing(JNIEnv* jvm, jobject connection) {
    void* pConnection = NULL;
    Connection* myConnection = NULL;

    GetPointer(jvm, connection, &pConnection);
    myConnection = (Connection*)pConnection;

    return (jboolean)(myConnection->state == AIO4C_CONNECTION_STATE_PENDING_CLOSE);
}
