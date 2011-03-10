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
#include <aio4c/buffer.h>
#include <aio4c/client.h>
#include <aio4c/connection.h>
#include <aio4c/event.h>
#include <aio4c/log.h>
#include <aio4c/thread.h>
#include <aio4c/types.h>
#include <aio4c/stats.h>

#include <errno.h>
#include <stdlib.h>
#include <string.h>
#include <sys/time.h>

void onRead(Connection* source) {
    Buffer* buffer = source->dataBuffer;
    struct timeval ping, pong;
    int mySeq = 0;

    if (memcmp(&buffer->data[buffer->position], "PONG ", 6) == 0) {
        buffer->position += 6;
        memcpy(&ping, &buffer->data[buffer->position], sizeof(struct timeval));
        buffer->position += sizeof(struct timeval);
        memcpy(&pong, &buffer->data[buffer->position], sizeof(struct timeval));
        buffer->position += sizeof(struct timeval);
        memcpy(&mySeq, &buffer->data[buffer->position], sizeof(int));
        buffer->position += sizeof(int);
        ProbeSize(AIO4C_PROBE_LATENCY_COUNT, 1);
        ProbeTime(AIO4C_TIME_PROBE_LATENCY, &ping, &pong);
    } else {
        Log(AIO4C_LOG_LEVEL_DEBUG, "unexpected answer %.*s from server on connection %s", 6, &buffer->data[buffer->position], source->string);
    }

    EnableWriteInterest(source);
}

void onWrite(Connection* source, int* seq) {
    Buffer* buffer = source->writeBuffer;
    struct timeval ping;

    (*seq) ++;

    gettimeofday(&ping, NULL);
    memcpy(&buffer->data[buffer->position], "PING ", 6);
    buffer->position += 6;
    memcpy(&buffer->data[buffer->position], &ping, sizeof(struct timeval));
    buffer->position += sizeof(struct timeval);
    memcpy(&buffer->data[buffer->position], seq, sizeof(int));
    buffer->position += sizeof(int);
}

void handler(Event event, Connection* source, int* seq) {
    switch (event) {
        case AIO4C_INBOUND_DATA_EVENT:
            onRead(source);
            break;
        case AIO4C_WRITE_EVENT:
            onWrite(source,seq);
            break;
        case AIO4C_CONNECTED_EVENT:
            EnableWriteInterest(source);
            break;
        default:
            break;
    }
}

int main (int argc, char* argv[]) {
    char* endptr = NULL;
    long int nbClients = 0;
    Client** clients = NULL;
    int i = 0;
    int* seq = NULL;

    if (argc != 2) {
        fprintf(stderr, "usage: client nbClient\n");
        exit(EXIT_FAILURE);
    }

    errno = 0;

    nbClients = strtol(argv[1], &endptr, 0);

    if (errno != 0) {
        perror("nbClient is not a valid integer");
        fprintf(stderr, "usage: client nbClient\n");
        exit(EXIT_FAILURE);
    }

    if (endptr == argv[1]) {
        fprintf(stderr, "nbClients must be an integer\n");
        fprintf(stderr, "usage: client nbClient\n");
        exit(EXIT_FAILURE);
    }

    printf("starting %ld clients...\n", nbClients);

    clients = malloc(nbClients * sizeof(Client*));
    seq = malloc(nbClients * sizeof(int));

    memset(clients, 0, nbClients * sizeof(Client*));
    memset(seq, 0, nbClients * sizeof(int));

    for (i = 0; i < nbClients; i++) {
        clients[i] = NewClient(i, AIO4C_ADDRESS_IPV4, "localhost", 11111, AIO4C_LOG_LEVEL_DEBUG, "client.log", 3, 3, 8192, aio4c_client_handler(handler), aio4c_client_handler_arg(&seq[i]));
        if (clients[i] == NULL) {
            printf("only %d clients were started !\n", i);
            break;
        }
    }

    if (i == nbClients) {
        printf ("all clients started !\n");
    }

    nbClients = i;

    for (i = 0; i < nbClients; i++) {
        if (clients[i] != NULL) {
            ClientEnd(clients[i]);
        }
    }

    LogEnd();

    free(seq);
    free(clients);

    return EXIT_SUCCESS;
}

