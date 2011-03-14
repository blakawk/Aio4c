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
#include <aio4c/jni/connection.h>

#include <aio4c/connection.h>
#include <aio4c/jni.h>

JNIEXPORT void JNICALL Java_com_aio4c_Connection_enableWriteInterest(JNIEnv* jvm, jobject connection) {
    Connection* myConnection = NULL;

    GetPointer(jvm, connection, (void**)&myConnection);

    EnableWriteInterest(myConnection);
}

JNIEXPORT jstring JNICALL Java_com_aio4c_Connection_toString(JNIEnv* jvm, jobject connection) {
    Connection* myConnection = NULL;
    jstring string = NULL;

    GetPointer(jvm, connection, (void**)&myConnection);

    string = (*jvm)->NewStringUTF(jvm, myConnection->string);

    return string;
}

JNIEXPORT void JNICALL Java_com_aio4c_Connection_close(JNIEnv* jvm, jobject connection) {
    Connection* myConnection = NULL;

    GetPointer(jvm, connection, (void**)&myConnection);

    ConnectionClose(myConnection);
}
