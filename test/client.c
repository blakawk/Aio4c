#include <aio4c/address.h>
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
} Client;

void onInit(Event event, Connection* source, Client* c) {
    Log(c->thread, INFO, "connection initialized");
    ConnectionConnect(source);
}

void onConnecting(Event event, Connection* source, Client* c) {
    struct pollfd polls[1] = { { .fd = source->socket, .events = POLLOUT, .revents = 0} };
    int soError = 0;
    socklen_t soLen = sizeof(soError);
    Log(c->thread, INFO, "connecting...");
    while (poll(polls, 1, -1) == 0);
    if (polls[0].revents == POLLOUT) {
        ConnectionFinishConnect(source);
    } else {
        if (polls[0].revents & POLLERR) {
            getsockopt(source->socket, SOL_SOCKET, SO_ERROR, &soError, &soLen);
            errno = soError;
            Log(c->thread, ERROR, "connecting [POLLERR]: %s", strerror(errno));
            ConnectionClose(source);
        } else if (polls[0].revents & POLLHUP) {
            Log(c->thread, ERROR, "disconnected while connecting");
            ConnectionClose(source);
        } else {
            Log(c->thread, ERROR, "connecting: %d: %s", errno, strerror(errno));
            ConnectionClose(source);
        }
    }
}

void onConnect(Event event, Connection* source, Client* c) {
    Log(c->thread, INFO, "connected");
    ReaderManageConnection(c->thread, c->reader, source);
}

void onRead(Event event, Connection* source, Client* c) {
    Log(c->thread, DEBUG, "read %d bytes", source->readBuffer->position);
    BufferFlip(source->readBuffer);
    LogBuffer(c->thread, DEBUG, source->readBuffer);
}

void onClose(Event event, Connection* source, Client* c) {
    if (source->closeCause != NO_ERROR) {
        Log(c->thread, ERROR, "connection unexpectedly closed for reason %d with code %d\n", source->closeCause, source->closeCode);
    } else {
        Log(c->thread, WARN, "connection closed");
    }
    ThreadCancel(c->thread, c->reader->thread, true);
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
    free(arg);
}

void clientInit(Client* c) {
    Log(c->thread, INFO, "started");
}

aio4c_bool_t clientRun(Client* c) {
    ConnectionAddHandler(c->conn, INIT_EVENT, aio4c_connection_handler(onInit), aio4c_connection_handler_arg(c), true);
    ConnectionAddHandler(c->conn, CONNECTING_EVENT, aio4c_connection_handler(onConnecting), aio4c_connection_handler_arg(c), true);
    ConnectionAddHandler(c->conn, CONNECTED_EVENT, aio4c_connection_handler(onConnect), aio4c_connection_handler_arg(c), true);
    ConnectionAddHandler(c->conn, READ_EVENT, aio4c_connection_handler(onRead), aio4c_connection_handler_arg(c), false);
    ConnectionAddHandler(c->conn, CLOSE_EVENT, aio4c_connection_handler(onClose), aio4c_connection_handler_arg(c), true);
    c->reader = NewReader(c->thread);
    Thread* readerT = c->reader->thread;
    ConnectionInit(c->conn);
    ThreadJoin(c->thread, readerT);
    FreeThread(c->thread, &readerT);
    FreeConnection(&c->conn);
    return false;
}

void clientExit(Client* c) {
    Log(c->thread, INFO, "exited");
    free(c);
}

int main (int argc, char** argv) {
    Address* addr = NewAddress(IPV4, "localhost", 11111);
    Connection* conn = NewConnection(8192, addr);
    Data* data = malloc(sizeof(Data));
    Client* client = malloc(sizeof(Client));
    client->conn = conn;
    client->thread = NULL;
    client->reader = NULL;
    data->thread = NULL;
    data->counter = 0;

    Thread* mainThread = ThreadMain("client");
    LogInit(mainThread, DEBUG, "client.log");

    Thread* testThread = data->thread = NewThread(mainThread, "test", (void(*)(void*))testInit, (aio4c_bool_t(*)(void*))testRun, (void(*)(void*))testExit, (void*)data);
    Thread* clientThread = client->thread = NewThread(mainThread, "client", aio4c_thread_handler(clientInit), aio4c_thread_run(clientRun), aio4c_thread_handler(clientExit), aio4c_thread_arg(client));

    ThreadJoin(mainThread, testThread);
    ThreadJoin(mainThread, clientThread);
    FreeThread(mainThread, &testThread);
    FreeThread(mainThread, &clientThread);
    LogEnd(mainThread);
    FreeThread(NULL, &mainThread);

    return EXIT_SUCCESS;
}

