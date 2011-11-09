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
#include "com_aio4c_Client.h"

#include <aio4c/alloc.h>
#include <aio4c/connection.h>
#include <aio4c/client.h>
#include <aio4c/jni.h>
#include <aio4c/stats.h>

typedef struct s_JavaClient {
    Client* client;
    JavaVM* jvm;
    jobject jClient;
    jobject jFactory;
    jobject jConnection;
} JavaClient;

static void _jniClientEventHandler(Event event, Connection* connection, ClientHandlerData _client) {
    ProbeTimeStart(AIO4C_TIME_PROBE_JNI_OVERHEAD);

    JavaClient* client = (JavaClient*)_client;
    jclass cConnection = NULL;
    jclass cFactory = NULL;
    void* pJvm = NULL;
    JNIEnv* jvm = NULL;
    jobject jFactory = client->jFactory;
    jobject jBuffer = NULL;
    jmethodID jMethod = NULL;

    (*client->jvm)->AttachCurrentThreadAsDaemon(client->jvm, pJvm, NULL);
    jvm = (JNIEnv*)pJvm;

    CheckJNICall(jvm, (*jvm)->GetObjectClass(jvm, jFactory), cFactory);

    if (client->jConnection != NULL) {
        CheckJNICall(jvm, (*jvm)->GetObjectClass(jvm, client->jConnection), cConnection);
    }


    switch (event) {
        case AIO4C_INIT_EVENT:
            CheckJNICall(jvm, (*jvm)->GetMethodID(jvm, cFactory, "create", "()Lcom/aio4c/Connection;"), jMethod);
            CheckJNICall(jvm, (*jvm)->CallObjectMethod(jvm, jFactory, jMethod), client->jConnection);
            CheckJNICall(jvm, (*jvm)->NewGlobalRef(jvm, client->jConnection), client->jConnection);
            ConnectionAddSystemHandler(connection, AIO4C_FREE_EVENT, aio4c_connection_handler(_jniClientEventHandler), aio4c_connection_handler_arg(client), true);
            SetPointer(jvm, client->jConnection, connection);
            CheckJNICall(jvm, (*jvm)->GetObjectClass(jvm, client->jConnection), cConnection);
            CheckJNICall(jvm, (*jvm)->GetMethodID(jvm, cConnection, "onInit", "()V"), jMethod);
            (*jvm)->CallVoidMethod(jvm, client->jConnection, jMethod);
            break;
        case AIO4C_CLOSE_EVENT:
            CheckJNICall(jvm, (*jvm)->GetMethodID(jvm, cConnection, "onClose", "()V"), jMethod);
            (*jvm)->CallVoidMethod(jvm, client->jConnection, jMethod);
            break;
        case AIO4C_CONNECTED_EVENT:
            CheckJNICall(jvm, (*jvm)->GetMethodID(jvm, cConnection, "onConnect", "()V"), jMethod);
            (*jvm)->CallVoidMethod(jvm, client->jConnection, jMethod);
            break;
        case AIO4C_READ_EVENT:
            CheckJNICall(jvm, (*jvm)->GetMethodID(jvm, cConnection, "onRead", "(Lcom/aio4c/buffer/Buffer;)V"), jMethod);
            jBuffer = New_com_aio4c_Buffer(jvm, connection->dataBuffer);
            (*jvm)->CallVoidMethod(jvm, client->jConnection, jMethod, jBuffer);
            break;
        case AIO4C_WRITE_EVENT:
            CheckJNICall(jvm, (*jvm)->GetMethodID(jvm, cConnection, "onWrite", "(Lcom/aio4c/buffer/Buffer;)V"), jMethod);
            jBuffer = New_com_aio4c_Buffer(jvm, connection->writeBuffer);
            (*jvm)->CallVoidMethod(jvm, client->jConnection, jMethod, jBuffer);
            break;
        case AIO4C_FREE_EVENT:
            (*jvm)->DeleteGlobalRef(jvm, client->jConnection);
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
    myJClient->client = NewClient(
            index,
            type,
            address,
            port,
            retries,
            retryInterval,
            bufferSize,
            _jniClientEventHandler,
            (ClientHandlerData)myJClient);

    (*jvm)->ReleaseStringUTFChars(jvm, oAddress, address);

    SetPointer(jvm, client, myJClient);

    ProbeTimeEnd(AIO4C_TIME_PROBE_JNI_OVERHEAD);
}

JNIEXPORT jboolean JNICALL Java_com_aio4c_Client_start(JNIEnv* jvm, jobject client) {
    void* pClient = NULL;
    JavaClient* cClient = NULL;

    GetPointer(jvm, client, &pClient);
    cClient = (JavaClient*)pClient;

    return (jboolean)ClientStart(cClient->client);
}

JNIEXPORT void JNICALL Java_com_aio4c_Client_join(JNIEnv* jvm, jobject client) {
    JavaClient* cClient = NULL;
    void* pClient = NULL;

    GetPointer(jvm, client, &pClient);
    cClient = (JavaClient*)pClient;

    ClientEnd(cClient->client);

    (*jvm)->DeleteGlobalRef(jvm, cClient->jClient);
    (*jvm)->DeleteGlobalRef(jvm, cClient->jFactory);

    aio4c_free(cClient);
}
