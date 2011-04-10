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
#include <aio4c/jni/client.h>

#include <aio4c.h>
#include <aio4c/jni.h>
#include <aio4c/connection.h>

typedef struct s_JavaClient {
    Client* client;
    JavaVM* jvm;
    jobject jClient;
    jobject jFactory;
    jobject jConnection;
} JavaClient;

static void _jniClientEventHandler(Event event, Connection* connection, JavaClient* client) {
    ProbeTimeStart(AIO4C_TIME_PROBE_JNI_OVERHEAD);

    jclass cConnection = NULL;
    jclass cFactory = NULL;
    JNIEnv* jvm = NULL;
    jobject jFactory = client->jFactory;
    jobject jBuffer = NULL;
    jmethodID jMethod = NULL;

    (*client->jvm)->AttachCurrentThreadAsDaemon(client->jvm, (void**)&jvm, NULL);

    cFactory = (*jvm)->GetObjectClass(jvm, jFactory);

    if (client->jConnection != NULL) {
        cConnection = (*jvm)->GetObjectClass(jvm, client->jConnection);
    }


    switch (event) {
        case AIO4C_INIT_EVENT:
            jMethod = (*jvm)->GetMethodID(jvm, cFactory, "create", "()Lcom/aio4c/Connection;");
            client->jConnection = (*jvm)->CallObjectMethod(jvm, jFactory, jMethod);
            client->jConnection = (*jvm)->NewGlobalRef(jvm, client->jConnection);
            SetPointer(jvm, client->jConnection, connection);
            cConnection = (*jvm)->GetObjectClass(jvm, client->jConnection);
            jMethod = (*jvm)->GetMethodID(jvm, cConnection, "onInit", "()V");
            (*jvm)->CallVoidMethod(jvm, client->jConnection, jMethod);
            break;
        case AIO4C_CLOSE_EVENT:
            jMethod = (*jvm)->GetMethodID(jvm, cConnection, "onClose", "()V");
            (*jvm)->CallVoidMethod(jvm, client->jConnection, jMethod);
            (*jvm)->DeleteGlobalRef(jvm, client->jConnection);
            break;
        case AIO4C_CONNECTED_EVENT:
            jMethod = (*jvm)->GetMethodID(jvm, cConnection, "onConnect", "()V");
            (*jvm)->CallVoidMethod(jvm, client->jConnection, jMethod);
            break;
        case AIO4C_INBOUND_DATA_EVENT:
            jMethod = (*jvm)->GetMethodID(jvm, cConnection, "onRead", "(Lcom/aio4c/Buffer;)V");
            jBuffer = New_com_aio4c_Buffer(jvm, connection->dataBuffer);
            (*jvm)->CallVoidMethod(jvm, client->jConnection, jMethod, jBuffer);
            break;
        case AIO4C_WRITE_EVENT:
            jMethod = (*jvm)->GetMethodID(jvm, cConnection, "onWrite", "(Lcom/aio4c/Buffer;)V");
            jBuffer = New_com_aio4c_Buffer(jvm, connection->writeBuffer);
            (*jvm)->CallVoidMethod(jvm, client->jConnection, jMethod, jBuffer);
            break;
        default:
            break;
    }

    ProbeTimeEnd(AIO4C_TIME_PROBE_JNI_OVERHEAD);
}

JNIEXPORT void JNICALL Java_com_aio4c_Client_initialize(JNIEnv* jvm, jobject client, jobject config, jobject factory) {
    ProbeTimeStart(AIO4C_TIME_PROBE_JNI_OVERHEAD);

    int index = 0, retries = 0, retryInterval = 0, bufferSize = 0, type = 0;
    jobject oType = NULL;
    jobject oAddress = NULL;
    short port = 0;
    char* address = NULL;
    jobject myClient = NULL;
    JavaClient* myJClient = NULL;

    GetField(jvm, config, "clientId", "I", &index);
    GetField(jvm, config, "type", "Lcom/aio4c/AddressType;", &oType);
    GetField(jvm, config, "address", "Ljava/lang/String;", &oAddress);
    GetField(jvm, config, "port", "S", &port);
    GetField(jvm, config, "retries", "I", &retries);
    GetField(jvm, config, "retryInterval", "I", &retryInterval);
    GetField(jvm, config, "bufferSize", "I", &bufferSize);
    GetField(jvm, oType, "value", "I", &type);

    if ((myJClient = aio4c_malloc(sizeof(JavaClient))) == NULL) {
        (*jvm)->ThrowNew(jvm, (*jvm)->FindClass(jvm, "Ljava/lang/RuntimeException;"), "cannot allocate memory (1)");
        return;
    }

    address = (char*)(*jvm)->GetStringUTFChars(jvm, (jstring)oAddress, NULL);
    if (address == NULL) {
        aio4c_free(myJClient);
        (*jvm)->ThrowNew(jvm, (*jvm)->FindClass(jvm, "Ljava/lang/RuntimeException;"), "cannot allocate memory (2)");
        return;
    }

    myClient = (*jvm)->NewGlobalRef(jvm, client);
    if (myClient == NULL) {
        aio4c_free(myJClient);
        (*jvm)->ReleaseStringUTFChars(jvm, oAddress, address);
        (*jvm)->ThrowNew(jvm, (*jvm)->FindClass(jvm, "Ljava/lang/RuntimeException;"), "cannot allocate memory (3)");
        return;
    }

    myJClient->jFactory = (*jvm)->NewGlobalRef(jvm, factory);
    if (myJClient->jFactory == NULL) {
        aio4c_free(myJClient);
        (*jvm)->DeleteGlobalRef(jvm, myClient);
        (*jvm)->ReleaseStringUTFChars(jvm, oAddress, address);
        (*jvm)->ThrowNew(jvm, (*jvm)->FindClass(jvm, "Ljava/lang/RuntimeException;"), "cannot allocate memory (4)");
        return;
    }

    (*jvm)->GetJavaVM(jvm, &myJClient->jvm);
    myJClient->jClient = myClient;
    myJClient->client = NewClient(index, type, address, port, retries, retryInterval, bufferSize, aio4c_client_handler(_jniClientEventHandler), aio4c_client_handler_arg(myJClient));

    (*jvm)->ReleaseStringUTFChars(jvm, oAddress, address);

    SetPointer(jvm, client, myJClient);

    ProbeTimeEnd(AIO4C_TIME_PROBE_JNI_OVERHEAD);
}

JNIEXPORT void JNICALL Java_com_aio4c_Client_join(JNIEnv* jvm, jobject client) {
    JavaClient* cClient = NULL;

    GetPointer(jvm, client, (void**)&cClient);

    ClientEnd(cClient->client);

    (*jvm)->DeleteGlobalRef(jvm, cClient->jClient);
    (*jvm)->DeleteGlobalRef(jvm, cClient->jFactory);

    aio4c_free(cClient);
}
