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
#include <aio4c.h>

#include <errno.h>
#include <limits.h>
#include <stdlib.h>
#include <string.h>
#include <sys/time.h>

static int maxRequests = 0;

void onRead(Connection* source) {
    Buffer* buffer = source->dataBuffer;
    struct timeval ping, pong, now;
    int mySeq = 0;

    LogBuffer(AIO4C_LOG_LEVEL_DEBUG, buffer);

    if (memcmp(&buffer->data[buffer->position], "PONG ", 6) == 0) {
        buffer->position += 6;
        BufferGet(buffer, &ping, sizeof(struct timeval));
        BufferGet(buffer, &pong, sizeof(struct timeval));
        BufferGetInt(buffer, &mySeq);
        gettimeofday(&now, NULL);
        ProbeSize(AIO4C_PROBE_LATENCY_COUNT, 1);
        ProbeTime(AIO4C_TIME_PROBE_LATENCY, &ping, &now);
        if (maxRequests != 0 && mySeq > maxRequests) {
            ConnectionClose(source, false);
        }
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
    BufferPutString(buffer, "PING ");
    BufferPut(buffer, &ping, sizeof(struct timeval));
    BufferPutInt(buffer, seq);

    BufferFlip(buffer);
    LogBuffer(AIO4C_LOG_LEVEL_DEBUG, buffer);
    buffer->position = buffer->limit;
    buffer->limit = buffer->size;
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

__attribute__((noreturn)) static void usage(char* arg0) {
    fprintf(stderr, "usage: %s [-Ch] [-Cn numClients] [-Cm maxRequests]\n", arg0);
    fprintf(stderr, "where:\n");
    fprintf(stderr, "\t-Cn numClients : defines the number of Clients to start (default: 1)\n");
    fprintf(stderr, "\t-Cm maxRequests: defines the maximum number of exchanges to perform (default: 10)\n");
    fprintf(stderr, "\t-Ch            : displays this message\n");
    Aio4cUsage();
    exit(EXIT_FAILURE);
}

int main (int argc, char* argv[]) {
    char* endptr = NULL;
    int nbClients = 1;
    Client** clients = NULL;
    int i = 0;
    int* seq = NULL;
    int optind = 0;
    long int optvalue = 0;

    for (optind = 1; optind < argc; optind++) {
        switch(argv[optind][0]) {
            case '-':
                switch (argv[optind][1]) {
                    case 'C':
                        switch (argv[optind][2]) {
                            case 'n':
                                if (optind + 1 < argc) {
                                    optvalue = strtol(argv[optind + 1], &endptr, 10);
                                    if (optvalue < INT_MAX && optvalue > 0) {
                                        nbClients = (int)optvalue;
                                    }
                                    optind++;
                                }
                                break;
                            case 'm':
                                if (optind + 1 < argc) {
                                    optvalue = strtol(argv[optind + 1], &endptr, 10);
                                    if (optvalue < INT_MAX && optvalue > 0) {
                                        maxRequests = (int)optvalue;
                                    }
                                    optind++;
                                }
                                break;
                            case 'h':
                                usage(argv[0]);
                                break;
                            default:
                                break;
                        }
                        break;
                    default:
                        break;
                }
                break;
            default:
                break;
        }
    }

    Aio4cInit(argc, argv);

    Log(AIO4C_LOG_LEVEL_DEBUG, "starting %d clients", nbClients);
    if (maxRequests) {
        Log(AIO4C_LOG_LEVEL_DEBUG, "each client will perform %d requests", maxRequests);
    } else {
        Log(AIO4C_LOG_LEVEL_DEBUG, "each client will perform unlimited number of requests");
    }

    clients = malloc(nbClients * sizeof(Client*));
    seq = malloc(nbClients * sizeof(int));

    memset(clients, 0, nbClients * sizeof(Client*));
    memset(seq, 0, nbClients * sizeof(int));

    for (i = 0; i < nbClients; i++) {
        clients[i] = NewClient(i, AIO4C_ADDRESS_IPV4, "localhost", 11111, 3, 3, 8192, aio4c_client_handler(handler), aio4c_client_handler_arg(&seq[i]));
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

    Aio4cEnd();

    free(seq);
    free(clients);

    return EXIT_SUCCESS;
}

