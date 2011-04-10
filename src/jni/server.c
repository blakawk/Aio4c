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
#include <aio4c/jni/server.h>

#include <aio4c.h>
#include <aio4c/jni.h>
#include <aio4c/connection.h>

typedef struct s_JavaServer {
    Server* server;
    JavaVM* jvm;
    jobject jServer;
    jobject jFactory;
    jobject jConnection;
} JavaServer;

static void _jniServerEventHandler(Event event, Connection* connection, JavaServer* server) {
    ProbeTimeStart(AIO4C_TIME_PROBE_JNI_OVERHEAD);

    jclass cConnection = NULL;
    JNIEnv* jvm = NULL;
    jobject jBuffer = NULL;
    jmethodID jMethod = NULL;

    (*server->jvm)->AttachCurrentThreadAsDaemon(server->jvm, (void**)&jvm, NULL);

    cConnection = (*jvm)->GetObjectClass(jvm, server->jConnection);

    switch (event) {
        case AIO4C_INIT_EVENT:
            jMethod = (*jvm)->GetMethodID(jvm, cConnection, "onInit", "()V");
            (*jvm)->CallVoidMethod(jvm, server->jConnection, jMethod);
            break;
        case AIO4C_CLOSE_EVENT:
            jMethod = (*jvm)->GetMethodID(jvm, cConnection, "onClose", "()V");
            (*jvm)->CallVoidMethod(jvm, server->jConnection, jMethod);
            (*jvm)->DeleteGlobalRef(jvm, server->jConnection);
            aio4c_free(server);
            break;
        case AIO4C_CONNECTED_EVENT:
            jMethod = (*jvm)->GetMethodID(jvm, cConnection, "onConnect", "()V");
            (*jvm)->CallVoidMethod(jvm, server->jConnection, jMethod);
            break;
        case AIO4C_INBOUND_DATA_EVENT:
            jMethod = (*jvm)->GetMethodID(jvm, cConnection, "onRead", "(Lcom/aio4c/Buffer;)V");
            jBuffer = New_com_aio4c_Buffer(jvm, connection->dataBuffer);
            (*jvm)->CallVoidMethod(jvm, server->jConnection, jMethod, jBuffer);
            break;
        case AIO4C_WRITE_EVENT:
            jMethod = (*jvm)->GetMethodID(jvm, cConnection, "onWrite", "(Lcom/aio4c/Buffer;)V");
            jBuffer = New_com_aio4c_Buffer(jvm, connection->writeBuffer);
            (*jvm)->CallVoidMethod(jvm, server->jConnection, jMethod, jBuffer);
            break;
        default:
            break;
    }

    ProbeTimeEnd(AIO4C_TIME_PROBE_JNI_OVERHEAD);
}

static JavaServer* _jniServerFactory(Connection* connection, JavaServer* server) {
    ProbeTimeStart(AIO4C_TIME_PROBE_JNI_OVERHEAD);

    JavaServer* _server = NULL;
    JNIEnv* jvm = NULL;
    jmethodID jMethod = NULL;
    jobject jFactory = server->jFactory;
    jclass cFactory = NULL;

    if ((_server = aio4c_malloc(sizeof(JavaServer))) == NULL) {
        return NULL;
    }

    memcpy(_server, server, sizeof(JavaServer));

    (*_server->jvm)->AttachCurrentThreadAsDaemon(server->jvm, (void**)&jvm, NULL);

    cFactory = (*jvm)->GetObjectClass(jvm, jFactory);

    jMethod = (*jvm)->GetMethodID(jvm, cFactory, "create", "()Lcom/aio4c/Connection;");
    _server->jConnection = (*jvm)->CallObjectMethod(jvm, jFactory, jMethod);
    _server->jConnection = (*jvm)->NewGlobalRef(jvm, _server->jConnection);
    SetPointer(jvm, _server->jConnection, connection);

    ProbeTimeEnd(AIO4C_TIME_PROBE_JNI_OVERHEAD);
    return _server;
}

JNIEXPORT void JNICALL Java_com_aio4c_Server_initialize(JNIEnv* jvm, jobject server, jobject config, jobject factory) {
    ProbeTimeStart(AIO4C_TIME_PROBE_JNI_OVERHEAD);

    int type = 0, bufferSize = 0, nbPipes = 0;
    jobject oType = 0;
    jobject oAddress = 0;
    short port = 0;
    char* address = 0;
    jobject myServer = NULL;
    JavaServer* myJServer = NULL;

    GetField(jvm, config, "type", "Lcom/aio4c/AddressType;", &oType);
    GetField(jvm, config, "address", "Ljava/lang/String;", &oAddress);
    GetField(jvm, config, "port", "S", &port);
    GetField(jvm, config, "bufferSize", "I", &bufferSize);
    GetField(jvm, config, "nbPipes", "I", &nbPipes);
    GetField(jvm, oType, "value", "I", &type);

    if ((myJServer = aio4c_malloc(sizeof(JavaServer))) == NULL) {
        (*jvm)->ThrowNew(jvm, (*jvm)->FindClass(jvm, "Ljava/lang/RuntimeException;"), "cannot allocate memory (5)");
        return;
    }

    address = (char*)(*jvm)->GetStringUTFChars(jvm, (jstring)oAddress, NULL);
    if (address == NULL) {
        aio4c_free(myJServer);
        (*jvm)->ThrowNew(jvm, (*jvm)->FindClass(jvm, "Ljava/lang/RuntimeException;"), "cannot allocate memory (6)");
        return;
    }

    myServer = (*jvm)->NewGlobalRef(jvm, server);
    if (myServer == NULL) {
        aio4c_free(myJServer);
        (*jvm)->ReleaseStringUTFChars(jvm, oAddress, address);
        (*jvm)->ThrowNew(jvm, (*jvm)->FindClass(jvm, "Ljava/lang/RuntimeException;"), "cannot allocate memory (7)");
        return;
    }

    myJServer->jFactory = (*jvm)->NewGlobalRef(jvm, factory);
    if (myJServer->jFactory == NULL) {
        aio4c_free(myJServer);
        (*jvm)->DeleteGlobalRef(jvm, myServer);
        (*jvm)->ReleaseStringUTFChars(jvm, oAddress, address);
        (*jvm)->ThrowNew(jvm, (*jvm)->FindClass(jvm, "Ljava/lang/RuntimeException;"), "cannot allocate memory (8)");
        return;
    }

    (*jvm)->GetJavaVM(jvm, &myJServer->jvm);
    myJServer->jServer = myServer;
    myJServer->server = NewServer(type, address, port, bufferSize, nbPipes, aio4c_server_handler(_jniServerEventHandler), aio4c_server_handler_arg(myJServer), aio4c_server_factory(_jniServerFactory));

    (*jvm)->ReleaseStringUTFChars(jvm, oAddress, address);

    SetPointer(jvm, server, myJServer);

    ProbeTimeEnd(AIO4C_TIME_PROBE_JNI_OVERHEAD);
}

JNIEXPORT void JNICALL Java_com_aio4c_Server_stop(JNIEnv* jvm, jobject server) {
    JavaServer* cServer = NULL;

    GetPointer(jvm, server, (void**)&cServer);

    ServerStop(cServer->server);
}

JNIEXPORT void JNICALL Java_com_aio4c_Server_join(JNIEnv* jvm, jobject server) {
    JavaServer* cServer = NULL;

    GetPointer(jvm, server, (void**)&cServer);

    ServerJoin(cServer->server);

    (*jvm)->DeleteGlobalRef(jvm, cServer->jServer);
    (*jvm)->DeleteGlobalRef(jvm, cServer->jFactory);

    aio4c_free(cServer);
}