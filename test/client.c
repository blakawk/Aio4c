#include <aio4c/address.h>
#include <aio4c/alloc.h>
#include <aio4c/buffer.h>
#include <aio4c/connection.h>
#include <aio4c/log.h>
#include <aio4c/reader.h>
#include <aio4c/thread.h>
#include <aio4c/types.h>

#include <errno.h>
#include <poll.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <unistd.h>

typedef struct s_Client {
    Thread* thread;
    Connection* conn;
    Reader* reader;
    Queue* queue;
    aio4c_bool_t freeOnClose;
} Client;

void onInit(Event event, Connection* source, Client* c) {
    if (event != INIT_EVENT) {
        return;
    }
    Log(c->thread, INFO, "connection initialized");
    ConnectionConnect(source);
}

void onConnect(Event event, Connection* source, Client* c) {
    if (event == CONNECTING_EVENT) {
        ConnectionFinishConnect(source);
        return;
    }

    c->freeOnClose = false;

    ReaderManageConnection(c->reader, source);
}

void onRead(Event event, Connection* source, Client* c) {
    if (event != INBOUND_DATA_EVENT) {
        return;
    }

    Log(c->thread, DEBUG, "read %d bytes", source->dataBuffer->limit);
    LogBuffer(c->thread, DEBUG, source->dataBuffer);
    source->dataBuffer->position = source->dataBuffer->limit;
    ConnectionEventHandle(source, OUTBOUND_DATA_EVENT, source);
}

void onWrite(Event event, Connection* source, Client* c) {
    if (event != WRITE_EVENT) {
        return;
    }

    memcpy(source->writeBuffer->data, "HELLO\n", 7);
    source->writeBuffer->limit = 7;
    Log(c->thread, DEBUG, "wrote %d bytes", 7);
    LogBuffer(c->thread, DEBUG, source->writeBuffer);
    source->writeBuffer->position += 7;
}

void onClose(Event event, Connection* source, Client* c) {
    if (event != CLOSE_EVENT || source->state != CLOSED) {
        return;
    }

    if (!EnqueueExitItem(c->queue)) {
        return;
    }
}

typedef struct s_Data {
    Thread* thread;
    int counter;
} Data;

void testInit(Data * arg) {
    Log(arg->thread, INFO, "test initialized");
    arg->counter = 0;
}

aio4c_bool_t testRun(Data* arg) {
    arg->counter++;
    Log(arg->thread, DEBUG, "test running %d", arg->counter);
    if (arg->counter > 10) {
        return false;
    }
    return true;
}

void testExit(Data* arg) {
    Log(arg->thread, INFO, "test exiting %d", arg->counter);
}

void clientInit(Client* c) {
    Log(c->thread, INFO, "started");
}

aio4c_bool_t clientRun(Client* c) {
    QueueItem item;
    memset(&item, 0, sizeof(QueueItem));
    ConnectionAddHandler(c->conn, INIT_EVENT, aio4c_connection_handler(onInit), aio4c_connection_handler_arg(c), true);
    ConnectionAddHandler(c->conn, CONNECTING_EVENT, aio4c_connection_handler(onConnect), aio4c_connection_handler_arg(c), true);
    ConnectionAddHandler(c->conn, CONNECTED_EVENT, aio4c_connection_handler(onConnect), aio4c_connection_handler_arg(c), true);
    ConnectionAddHandler(c->conn, INBOUND_DATA_EVENT, aio4c_connection_handler(onRead), aio4c_connection_handler_arg(c), false);
    ConnectionAddHandler(c->conn, WRITE_EVENT, aio4c_connection_handler(onWrite), aio4c_connection_handler_arg(c), false);
    ConnectionAddHandler(c->conn, CLOSE_EVENT, aio4c_connection_handler(onClose), aio4c_connection_handler_arg(c), true);
    c->freeOnClose = true;
    c->reader = NewReader(c->thread, "reader", 8192);
    ConnectionInit(c->conn);
    Dequeue(c->queue, &item, true);
    if (c->freeOnClose) {
        FreeConnection(&c->conn);
    }
    ReaderEnd(c->reader);
    return false;
}

void clientExit(Client* c) {
    Log(c->thread, INFO, "exited");
    FreeQueue(&c->queue);
}

int main (void) {
    Address* addr = NewAddress(IPV4, "srvotc", 11111);
    BufferPool* pool = NewBufferPool(2, 8192);
    Connection* conn = NewConnection(AllocateBuffer(pool), AllocateBuffer(pool), addr);
    Data* data = aio4c_malloc(sizeof(Data));
    Client* client = aio4c_malloc(sizeof(Client));
    client->conn = conn;
    client->thread = NULL;
    client->reader = NULL;
    data->thread = NULL;
    data->counter = 0;

    Thread* mainThread = ThreadMain("main");
    LogInit(mainThread, INFO, "client.log");

    client->queue = NewQueue();
    data->thread = NewThread("test", (void(*)(void*))testInit, (aio4c_bool_t(*)(void*))testRun, (void(*)(void*))testExit, (void*)data);
    client->thread = NewThread("client", aio4c_thread_handler(clientInit), aio4c_thread_run(clientRun), aio4c_thread_handler(clientExit), aio4c_thread_arg(client));

    ThreadJoin(data->thread);
    FreeThread(&data->thread);
    aio4c_free(data);
    ThreadJoin(client->thread);
    FreeThread(&client->thread);
    aio4c_free(client);
    LogEnd();
    FreeThread(&mainThread);
    FreeBufferPool(&pool);

    return EXIT_SUCCESS;
}

