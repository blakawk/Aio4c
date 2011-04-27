/**
 * This file is part of Aio4c <http://aio4c.so>.
 *
 * Aio4c <http://aio4c.so> is free software: you
 * can  redistribute  it  and/or modify it under
 * the  terms  of the GNU General Public License
 * as published by the Free Software Foundation,
 * version 3 of the License.
 *
 * This  program is distributed in the hope that
 * it  will be useful, but WITHOUT ANY WARRANTY;
 * without   even   the   implied   warranty  of
 * MERCHANTABILITY  or  FITNESS FOR A PARTICULAR
 * PURPOSE.
 *
 * See  the  GNU General Public License for more
 * details.  You  should have received a copy of
 * the  GNU  General  Public  License along with
 * Aio4c    <http://aio4c.so>.   If   not,   see
 * <http://www.gnu.org/licenses/>.
 */
#include <aio4c.h>

#include <assert.h>
#include <fcntl.h>
#include <limits.h>
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>
#include <unistd.h>

#ifndef AIO4C_WIN32
#include <sys/wait.h>
#endif /* AIO4C_WIN32 */

// CRC-32 Algorithm retrieved from:
// http://pubs.opengroup.org/onlinepubs/009695399/utilities/cksum.html
// Alls right reserved to The IEEE and The Open Group
static unsigned int crctab[] = {
    0x00000000,
    0x04c11db7, 0x09823b6e, 0x0d4326d9, 0x130476dc, 0x17c56b6b,
    0x1a864db2, 0x1e475005, 0x2608edb8, 0x22c9f00f, 0x2f8ad6d6,
    0x2b4bcb61, 0x350c9b64, 0x31cd86d3, 0x3c8ea00a, 0x384fbdbd,
    0x4c11db70, 0x48d0c6c7, 0x4593e01e, 0x4152fda9, 0x5f15adac,
    0x5bd4b01b, 0x569796c2, 0x52568b75, 0x6a1936c8, 0x6ed82b7f,
    0x639b0da6, 0x675a1011, 0x791d4014, 0x7ddc5da3, 0x709f7b7a,
    0x745e66cd, 0x9823b6e0, 0x9ce2ab57, 0x91a18d8e, 0x95609039,
    0x8b27c03c, 0x8fe6dd8b, 0x82a5fb52, 0x8664e6e5, 0xbe2b5b58,
    0xbaea46ef, 0xb7a96036, 0xb3687d81, 0xad2f2d84, 0xa9ee3033,
    0xa4ad16ea, 0xa06c0b5d, 0xd4326d90, 0xd0f37027, 0xddb056fe,
    0xd9714b49, 0xc7361b4c, 0xc3f706fb, 0xceb42022, 0xca753d95,
    0xf23a8028, 0xf6fb9d9f, 0xfbb8bb46, 0xff79a6f1, 0xe13ef6f4,
    0xe5ffeb43, 0xe8bccd9a, 0xec7dd02d, 0x34867077, 0x30476dc0,
    0x3d044b19, 0x39c556ae, 0x278206ab, 0x23431b1c, 0x2e003dc5,
    0x2ac12072, 0x128e9dcf, 0x164f8078, 0x1b0ca6a1, 0x1fcdbb16,
    0x018aeb13, 0x054bf6a4, 0x0808d07d, 0x0cc9cdca, 0x7897ab07,
    0x7c56b6b0, 0x71159069, 0x75d48dde, 0x6b93dddb, 0x6f52c06c,
    0x6211e6b5, 0x66d0fb02, 0x5e9f46bf, 0x5a5e5b08, 0x571d7dd1,
    0x53dc6066, 0x4d9b3063, 0x495a2dd4, 0x44190b0d, 0x40d816ba,
    0xaca5c697, 0xa864db20, 0xa527fdf9, 0xa1e6e04e, 0xbfa1b04b,
    0xbb60adfc, 0xb6238b25, 0xb2e29692, 0x8aad2b2f, 0x8e6c3698,
    0x832f1041, 0x87ee0df6, 0x99a95df3, 0x9d684044, 0x902b669d,
    0x94ea7b2a, 0xe0b41de7, 0xe4750050, 0xe9362689, 0xedf73b3e,
    0xf3b06b3b, 0xf771768c, 0xfa325055, 0xfef34de2, 0xc6bcf05f,
    0xc27dede8, 0xcf3ecb31, 0xcbffd686, 0xd5b88683, 0xd1799b34,
    0xdc3abded, 0xd8fba05a, 0x690ce0ee, 0x6dcdfd59, 0x608edb80,
    0x644fc637, 0x7a089632, 0x7ec98b85, 0x738aad5c, 0x774bb0eb,
    0x4f040d56, 0x4bc510e1, 0x46863638, 0x42472b8f, 0x5c007b8a,
    0x58c1663d, 0x558240e4, 0x51435d53, 0x251d3b9e, 0x21dc2629,
    0x2c9f00f0, 0x285e1d47, 0x36194d42, 0x32d850f5, 0x3f9b762c,
    0x3b5a6b9b, 0x0315d626, 0x07d4cb91, 0x0a97ed48, 0x0e56f0ff,
    0x1011a0fa, 0x14d0bd4d, 0x19939b94, 0x1d528623, 0xf12f560e,
    0xf5ee4bb9, 0xf8ad6d60, 0xfc6c70d7, 0xe22b20d2, 0xe6ea3d65,
    0xeba91bbc, 0xef68060b, 0xd727bbb6, 0xd3e6a601, 0xdea580d8,
    0xda649d6f, 0xc423cd6a, 0xc0e2d0dd, 0xcda1f604, 0xc960ebb3,
    0xbd3e8d7e, 0xb9ff90c9, 0xb4bcb610, 0xb07daba7, 0xae3afba2,
    0xaafbe615, 0xa7b8c0cc, 0xa379dd7b, 0x9b3660c6, 0x9ff77d71,
    0x92b45ba8, 0x9675461f, 0x8832161a, 0x8cf30bad, 0x81b02d74,
    0x857130c3, 0x5d8a9099, 0x594b8d2e, 0x5408abf7, 0x50c9b640,
    0x4e8ee645, 0x4a4ffbf2, 0x470cdd2b, 0x43cdc09c, 0x7b827d21,
    0x7f436096, 0x7200464f, 0x76c15bf8, 0x68860bfd, 0x6c47164a,
    0x61043093, 0x65c52d24, 0x119b4be9, 0x155a565e, 0x18197087,
    0x1cd86d30, 0x029f3d35, 0x065e2082, 0x0b1d065b, 0x0fdc1bec,
    0x3793a651, 0x3352bbe6, 0x3e119d3f, 0x3ad08088, 0x2497d08d,
    0x2056cd3a, 0x2d15ebe3, 0x29d4f654, 0xc5a92679, 0xc1683bce,
    0xcc2b1d17, 0xc8ea00a0, 0xd6ad50a5, 0xd26c4d12, 0xdf2f6bcb,
    0xdbee767c, 0xe3a1cbc1, 0xe760d676, 0xea23f0af, 0xeee2ed18,
    0xf0a5bd1d, 0xf464a0aa, 0xf9278673, 0xfde69bc4, 0x89b8fd09,
    0x8d79e0be, 0x803ac667, 0x84fbdbd0, 0x9abc8bd5, 0x9e7d9662,
    0x933eb0bb, 0x97ffad0c, 0xafb010b1, 0xab710d06, 0xa6322bdf,
    0xa2f33668, 0xbcb4666d, 0xb8757bda, 0xb5365d03, 0xb1f740b4
};

static unsigned int crc32(unsigned char* b, size_t n) {
    unsigned int i = 0, c = 0, s = 0;

    for (i = n; i > 0; --i) {
        c = (unsigned int)(*b++);
        s = (s << 8) ^ crctab[(s >> 24) ^ c];
    }

    while (n != 0) {
        c = n & 0377;
        n >>= 8;
        s = (s << 8) ^ crctab[(s >> 24) ^ c];
    }

    return ~s;
}
// End of CRC-32 algorithm

#define BUFSZ (blockSize + sizeof(struct timeval) + sizeof(int))

static int blockSize = 4096;
static unsigned long int clientDataSize = 1073741824;

static void random(unsigned char* data) {
    static int initialized = 0;
    int i = 0;

    if (!initialized) {
        srand(getpid());
        initialized = 1;
    }

    for (i = 0; i < blockSize; i++) {
        data[i] = rand() & 0xff;
    }
}

typedef struct s_ClientData {
    unsigned long int exchanged;
    unsigned char finished;
} ClientData;

static void clientHandler(Event event, Connection* connection, ClientData* cd) {
    Buffer* buf = NULL;
    unsigned int crc = 0, ck = 0;
    unsigned char* data = NULL;
    struct timeval now, req;
    double percent = 0.0;
    int i = 0;
    data = malloc(blockSize + sizeof(struct timeval));
    memset(data, 0, blockSize + sizeof(struct timeval));

    switch(event) {
        case AIO4C_CONNECTED_EVENT:
            EnableWriteInterest(connection);
            break;
        case AIO4C_WRITE_EVENT:
            buf = connection->writeBuffer;
            random(data);
            gettimeofday((struct timeval*)&data[blockSize - 1], NULL);
            BufferPut(buf, data, blockSize + sizeof(struct timeval));
            crc = crc32(data, blockSize + sizeof(struct timeval));
            Log(AIO4C_LOG_LEVEL_DEBUG, "sending CRC %u", crc);
            BufferPutInt(buf, &crc);
            break;
        case AIO4C_INBOUND_DATA_EVENT:
            buf = connection->dataBuffer;
            BufferGet(buf, data, blockSize);
            BufferGet(buf, &req, sizeof(struct timeval));
            memcpy(&data[blockSize], &req, sizeof(struct timeval));
            BufferGetInt(buf, &crc);
            ck = crc32(data, blockSize + sizeof(struct timeval));
            gettimeofday(&now, NULL);
            ProbeSize(AIO4C_PROBE_LATENCY_COUNT, 1);
            ProbeTime(AIO4C_TIME_PROBE_LATENCY, &req, &now);
            if (ck != crc) {
                Log(AIO4C_LOG_LEVEL_DEBUG, "checksum error (received: %u, computed: %u)", ck, crc);
                ConnectionClose(connection, false);
            } else if (cd->exchanged >= (unsigned long int)blockSize) {
                cd->exchanged -= blockSize;
                EnableWriteInterest(connection);
                percent = (((double)clientDataSize) - ((double)(cd->exchanged))) * 100.0 / ((double)clientDataSize);
                printf("\r[");
                for (i = 1; i <= 100; i++) {
                    if (percent >= i) {
                        printf("=");
                    } else if (percent >= i - 1 && percent <= i) {
                        printf(">");
                    } else {
                        printf(".");
                    }
                }
                printf("] %06.2lf %%", percent);
                fflush(stdout);
            } else {
                cd->finished = 1;
                ConnectionClose(connection, false);
            }
            break;
        default:
            break;
    }

    free(data);
}

static void serverHandler(Event event, Connection* connection, unsigned char* data) {
    Buffer* buf = NULL;
    unsigned int crc = 0, ck = 0;
    unsigned int* _crc = NULL;

    switch(event) {
        case AIO4C_WRITE_EVENT:
            buf = ConnectionGetWriteBuffer(connection);
            BufferPut(buf, data, BUFSZ);
            break;
        case AIO4C_INBOUND_DATA_EVENT:
            buf = ConnectionGetReadBuffer(connection);
            BufferGet(buf, data, BUFSZ);
            _crc = (unsigned int*)&data[blockSize + sizeof(struct timeval)];
            crc = *_crc;
            ck = crc32((unsigned char*)data, blockSize + sizeof(struct timeval));
            Log(AIO4C_LOG_LEVEL_DEBUG, "received CRC: %u, computed: %u", crc, ck);
            if (ck != crc) {
                ConnectionClose(connection, false);
            } else {
                EnableWriteInterest(connection);
            }
            break;
        case AIO4C_FREE_EVENT:
            free(data);
            break;
        default:
            break;
    }
}

static void* serverFactory(Connection* connection __attribute__((unused)), void* dummy __attribute__((unused))) {
    return malloc(BUFSZ);
}

__attribute__((noreturn)) static void usage(char* argv0) {
#ifndef AIO4C_WIN32
    fprintf(stderr, "usage: %s [-Cn numClients] [-Sn numPipes] [-h] [--client|--server]\n", argv0);
#else /* AIO4C_WIN32 */
    fprintf(stderr, "usage: %s --client|--server [-Cn numClients] [-Sn numPipes] [-h]\n", argv0);
#endif /* AIO4C_WIN32 */
    fprintf(stderr, "          [-Ch hostname] [-Cp port] [-Sh hostname] [-Sp port] [-Cs size]\n");
    fprintf(stderr, "          [-B block]\n");
    fprintf(stderr, "where:\n");
    fprintf(stderr, "\t-Cn numClients: defines the number of client to launch (default: 1)\n");
    fprintf(stderr, "\t-Ch hostname  : defines the address where to connect the client (default: localhost)\n");
    fprintf(stderr, "\t-Cp port      : defines the TCP port number where to connect the client (default: 11111)\n");
    fprintf(stderr, "\t-Cs size      : defines the amount of data to exchange in byte (default: 1073741824 bytes = 1gb)\n");
    fprintf(stderr, "\t-Sn numPipes  : defines the number of pipes to use for server (default: 1)\n");
    fprintf(stderr, "\t-Sh hostname  : defines the address for the server to listen on (default: localhost)\n");
    fprintf(stderr, "\t-Sp port      : defines the port for the server to listen on (default: 11111)\n");
    fprintf(stderr, "\t--client      : the program will run in client mode only\n");
    fprintf(stderr, "\t--server      : the program will run in server mode only\n");
    fprintf(stderr, "\t-h            : displays this help message\n");
    fprintf(stderr, "\t-B block      : defines the data block size (default: 4096 bytes)\n");
    Aio4cUsage();
    exit(EXIT_SUCCESS);
}

static Server* server = NULL;

static void sigint(int signal __attribute__((unused))) {
    if (server != NULL) {
        ServerStop(server);
    }
}

int main(int argc, char* argv[]) {
    int optind = 0;
    long int optvalue = 0;
    char* endptr = NULL;
    int nbPipes = 1;
    int nbClients = 1;
    char* clientHost = "localhost";
    char* serverHost = "localhost";
    int clientPort = 11111;
    int serverPort = 11111;
    Client** clients = NULL;
    ClientData* cds = NULL;
    int i = 0;
    unsigned char mode = 0;
    unsigned char success = 1;
#ifndef AIO4C_WIN32
    char** args = NULL;
    pid_t clientPid = 0, serverPid = 0;
#endif /* AIO4C_WIN32 */

    for (optind = 1; optind < argc; optind++) {
        switch (argv[optind][0]) {
            case '-':
                switch (argv[optind][1]) {
                    case 'C':
                        switch (argv[optind][2]) {
                            case 'n':
                                if (optind + 1 < argc) {
                                    optvalue = strtol(argv[optind + 1], &endptr, 10);
                                    if (optvalue > 0 && optvalue < INT_MAX) {
                                        nbClients = (int)optvalue;
                                    }
                                    optind++;
                                }
                                break;
                            case 'h':
                                if (optind + 1 < argc) {
                                    clientHost = argv[optind + 1];
                                    optind++;
                                }
                                break;
                            case 'p':
                                if (optind + 1 < argc) {
                                    optvalue = strtol(argv[optind + 1], &endptr, 10);
                                    if (optvalue > 0 && optvalue < INT_MAX) {
                                        clientPort = (int)optvalue;
                                    }
                                    optind++;
                                }
                                break;
                            case 's':
                                if (optind + 1 < argc) {
                                    clientDataSize = strtoul(argv[optind + 1], &endptr, 10);
                                    optind++;
                                }
                                break;
                            default:
                                break;
                        }
                        break;
                    case 'S':
                        switch (argv[optind][2]) {
                            case 'n':
                                if (optind + 1 < argc) {
                                    optvalue = strtol(argv[optind + 1], &endptr, 10);
                                    if (optvalue > 0 && optvalue < INT_MAX) {
                                        nbPipes = (int)optvalue;
                                    }
                                    optind++;
                                }
                                break;
                            case 'h':
                                if (optind + 1 < argc) {
                                    serverHost = argv[optind + 1];
                                    optind++;
                                }
                                break;
                            case 'p':
                                if (optind + 1 < argc) {
                                    optvalue = strtol(argv[optind + 1], &endptr, 10);
                                    if (optvalue > 0 && optvalue < INT_MAX) {
                                        serverPort = (int)optvalue;
                                    }
                                    optind++;
                                }
                                break;
                            default:
                                break;
                        }
                        break;
                    case 'h':
                        usage(argv[0]);
                        break;
                    case 'B':
                        if (optind + 1 < argc) {
                            optvalue = strtol(argv[optind + 1], &endptr, 10);
                            if (optvalue > 0 && optvalue < INT_MAX) {
                                blockSize = (int)optvalue;
                            }
                            optind++;
                        }
                        break;
                    case '-':
                        if (mode == 0) {
                            if (memcmp(argv[optind], "--client", 8) == 0) {
                                mode = 1;
                            } else if (memcmp(argv[optind], "--server", 8) == 0) {
                                mode = 2;
                            }
                        }
                    default:
                        break;
                }
                break;
            default:
                break;
        }
    }

    if (mode) {
        Aio4cInit(argc, argv);
    }

    switch (mode) {
        case 1:
            clients = malloc(sizeof(Client*) * nbClients);
            cds = malloc(sizeof(ClientData) * nbClients);
            for (i = 0; i < nbClients; i++) {
                cds[i].exchanged = clientDataSize;
                cds[i].finished = 0;
                clients[i] = NewClient(i, AIO4C_ADDRESS_IPV4, clientHost, clientPort, 3, 3, BUFSZ, aio4c_client_handler(clientHandler), aio4c_client_handler_arg(&cds[i]));
            }
            success = 1;
            for (i = 0; i < nbClients; i++) {
                ClientEnd(clients[i]);
                success = success && cds[i].finished;
            }
            if (success) {
                printf(" OK\n");
            } else {
                printf(" KO\n");
            }
            free(clients);
            free(cds);
            break;
        case 2:
            signal(SIGINT, sigint);
            server = NewServer(AIO4C_ADDRESS_IPV4, serverHost, serverPort, BUFSZ, nbPipes, aio4c_server_handler(serverHandler), NULL, serverFactory);
            ServerJoin(server);
            break;
        case 0:
#ifndef AIO4C_WIN32
            args = malloc(sizeof(char*) * (argc + 2));
            for (i = 0; i < argc; i++) {
                args[i] = argv[i];
            }
            args[argc + 1] = NULL;
            args[argc] = "--server";
            switch ((serverPid = fork())) {
                case 0:
                    execvp(argv[0], args);
                    break;
                case -1:
                    exit(EXIT_FAILURE);
                    break;
                default:
                    args[argc] = "--client";
                    switch ((clientPid = fork())) {
                        case 0:
                            execvp(argv[0], args);
                            exit(EXIT_FAILURE);
                            break;
                        case -1:
                            exit(EXIT_FAILURE);
                            break;
                        default:
                            break;
                    }
                    break;
            }
            free(args);
            waitpid(clientPid, NULL, 0);
            kill(serverPid, SIGINT);
            waitpid(serverPid, NULL, 0);
#else /* AIO4C_WIN32 */
            fprintf(stderr, "error: please specify either --client or --server mode\n");
            exit(EXIT_FAILURE);
#endif /* AIO4C_WIN32 */
            break;
        default:
            break;
    }

    if (mode) {
        Aio4cEnd();
    }

    if (mode == 1) {
        if (success) {
            exit(EXIT_SUCCESS);
        } else {
            exit(EXIT_FAILURE);
        }
    } else {
        exit(EXIT_SUCCESS);
    }
}
