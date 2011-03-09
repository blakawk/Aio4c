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

#include <aio4c/client.h>
#include <aio4c/alloc.h>
#include <aio4c/log.h>
#include <aio4c/stats.h>

#define p2j(p) \
    ((jlong)(AIO4C_P_TYPE)(p))

#define j2p(j,p) \
    ((p)(AIO4C_P_TYPE)(j))

typedef struct s_JavaClient {
    Client* client;
    JavaVM* jvm;
    jobject jClient;
} JavaClient;

static void _jniClientEventHandler(Event event, Connection* connection, JavaClient* client) {
    jclass cClient = NULL;
    JNIEnv* jvm = NULL;
    jobject jClient = client->jClient;
    jmethodID jMethod = NULL;
    jbyteArray bArray = NULL;
    jint writed = 0;

    (*client->jvm)->AttachCurrentThreadAsDaemon(client->jvm, (void**)&jvm, NULL);

    cClient = (*jvm)->GetObjectClass(jvm, jClient);

    switch (event) {
        case AIO4C_INIT_EVENT:
            jMethod = (*jvm)->GetMethodID(jvm, cClient, "onInit", "()V");
            (*jvm)->CallVoidMethod(jvm, jClient, jMethod);
            break;
        case AIO4C_CLOSE_EVENT:
            jMethod = (*jvm)->GetMethodID(jvm, cClient, "onClose", "()V");
            (*jvm)->CallVoidMethod(jvm, jClient, jMethod);
            break;
        case AIO4C_CONNECTED_EVENT:
            jMethod = (*jvm)->GetMethodID(jvm, cClient, "onConnect", "()V");
            (*jvm)->CallVoidMethod(jvm, jClient, jMethod);
            break;
        case AIO4C_INBOUND_DATA_EVENT:
            jMethod = (*jvm)->GetMethodID(jvm, cClient, "processData", "([B)V");
            bArray = (*jvm)->NewByteArray(jvm, connection->dataBuffer->limit - connection->dataBuffer->position);
            (*jvm)->SetByteArrayRegion(jvm, bArray, 0, connection->dataBuffer->limit - connection->dataBuffer->position, (jbyte*)&connection->dataBuffer->data[connection->dataBuffer->position]);
            (*jvm)->CallVoidMethod(jvm, jClient, jMethod, bArray);
            break;
        case AIO4C_WRITE_EVENT:
            jMethod = (*jvm)->GetMethodID(jvm, cClient, "writeData", "([B)I");
            bArray = (*jvm)->NewByteArray(jvm, connection->writeBuffer->limit - connection->writeBuffer->position);
            writed = (*jvm)->CallIntMethod(jvm, jClient, jMethod, bArray);
            (*jvm)->GetByteArrayRegion(jvm, bArray, 0, connection->writeBuffer->limit - connection->writeBuffer->position, (jbyte*)&connection->writeBuffer->data[connection->writeBuffer->position]);
            connection->writeBuffer->position += writed;
            break;
        default:
            break;
    }

}

JNIEXPORT jlong JNICALL Java_com_aio4c_Client_Create(JNIEnv *jvm, jobject client) {
    ProbeTimeStart(AIO4C_TIME_PROBE_JNI_OVERHEAD);
    int addressType = 0, retries = 0, retryInterval = 0, bufferSize = 0, logLevel = 0;
    short port = 0;
    char *address = NULL, *log = NULL;
    jfieldID jAddressType = NULL, jRetries = NULL, jRetryInterval = NULL, jBufferSize = NULL, jPort = NULL, jAddress = NULL, jLogLevel = NULL, jLog = NULL;
    jclass jClient = NULL;
    jobject oAddress = NULL;
    jobject oLog = NULL;
    jlong result = 0L;
    jobject myClient = NULL;
    JavaClient* myJClient = NULL;

    jClient = (*jvm)->GetObjectClass(jvm, client);
    jAddressType = (*jvm)->GetFieldID(jvm, jClient, "addressType", "I");
    jRetries = (*jvm)->GetFieldID(jvm, jClient, "retries", "I");
    jRetryInterval = (*jvm)->GetFieldID(jvm, jClient, "retryInterval", "I");
    jBufferSize = (*jvm)->GetFieldID(jvm, jClient, "bufferSize", "I");
    jPort = (*jvm)->GetFieldID(jvm, jClient, "port", "S");
    jAddress = (*jvm)->GetFieldID(jvm, jClient, "address", "Ljava/lang/String;");
    jLog = (*jvm)->GetFieldID(jvm, jClient, "log", "Ljava/lang/String;");
    jLogLevel = (*jvm)->GetFieldID(jvm, jClient, "logLevel", "I");

    addressType = (int)(*jvm)->GetIntField(jvm, client, jAddressType);
    retries = (int)(*jvm)->GetIntField(jvm, client, jRetries);
    retryInterval = (int)(*jvm)->GetIntField(jvm, client, jRetryInterval);
    bufferSize = (int)(*jvm)->GetIntField(jvm, client, jBufferSize);
    port = (short)(*jvm)->GetShortField(jvm, client, jPort);
    oAddress = (*jvm)->GetObjectField(jvm, client, jAddress);
    oLog = (*jvm)->GetObjectField(jvm, client, jLog);
    logLevel = (*jvm)->GetIntField(jvm, client, jLogLevel);

    address = (char*)(*jvm)->GetStringUTFChars(jvm, (jstring)oAddress, NULL);
    log = (char*)(*jvm)->GetStringUTFChars(jvm, (jstring)oLog, NULL);

    myClient = (*jvm)->NewGlobalRef(jvm, client);

    myJClient = aio4c_malloc(sizeof(JavaClient));
    (*jvm)->GetJavaVM(jvm, &myJClient->jvm);
    myJClient->jClient = myClient;

    result = p2j(myJClient->client = NewClient(addressType, address, port, logLevel, log, retries, retryInterval, bufferSize, aio4c_client_handler(_jniClientEventHandler), aio4c_client_handler_arg(myJClient)));

    (*jvm)->ReleaseStringUTFChars(jvm, oAddress, address);
    (*jvm)->ReleaseStringUTFChars(jvm, oLog, log);

    return result;
    ProbeTimeEnd(AIO4C_TIME_PROBE_JNI_OVERHEAD);
}

JNIEXPORT void JNICALL Java_com_aio4c_Client_End(JNIEnv* jvm, jobject client) {
    Client* cClient = NULL;
    Thread* tClient = NULL;
    jclass jClient = NULL;
    jfieldID fClient = NULL;

    jClient = (*jvm)->GetObjectClass(jvm, client);
    fClient = (*jvm)->GetFieldID(jvm, jClient, "pClient", "J");
    cClient = j2p((*jvm)->GetLongField(jvm, client, fClient),Client*);

    tClient = cClient->thread;
    ThreadJoin(tClient);
}
