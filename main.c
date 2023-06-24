#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <signal.h>
#include <errno.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <time.h>

#include "main.h"
#include "debug.h"
#include "config.h"
#include "file.h"
#include "packet.h"
#include "hashmap.h"
#include "id.h"

#define BUFFER_SIZE 4196

char* dnsServerIp = "192.168.1.1";
char* hijackFilepath = "./dns.conf";

int stop = 0;

int main(int argc, char *argv[]) {

    // 初始化
    if (argc > 4 || (argc > 3 && argv[1][0] != '-')) {
        printf("Usage: dns [-d|-dd] [dns-server-ip] [hijackFilepath]\n");
        return 1;
    }
    if (argc > 1) {
        int debugLevelOne = strcmp(argv[1], "-d");
        int debugLevelTwo = strcmp(argv[1], "-dd");

        if (debugLevelOne == 0 || debugLevelTwo == 0) {
            setDebugLevel(debugLevelTwo == 0 ? 2 : 1);
            if (argc > 2) {
                dnsServerIp = argv[2];
            }
            if (argc > 3) {
                hijackFilepath = argv[3];
            }
        } else {
            dnsServerIp = argv[1];
            if (argc > 2) {
                hijackFilepath = argv[2];
            }
        }
    }
    printf("Start with debugLevel = %d, dnsServerIp = %s, hijackFilepath = %s\n", debugLevel, dnsServerIp, hijackFilepath);
    ddbg("Config is ready")

    doDns();
    return 0;
}

void doDns() {
    char* hijackFileContent = readAllContent(hijackFilepath);
    dbg("Read from hijackFilePath : \n%s", hijackFileContent)

    free(hijackFileContent);
    ddbg("Free hijackFileContent")

    dbg("Start create udp server")
    int s = socket(AF_INET,SOCK_DGRAM,0);
    if (s < 0) {
        printf("Create socket fail, code %d %s", errno, strerror(errno));
        exit(errno);
    }
    dbg("Create socket success")
    struct sockaddr_in sockaddr;
    memset(&sockaddr, 0, sizeof(sockaddr));
    sockaddr.sin_family = AF_INET;
    sockaddr.sin_port = htons(53);
    sockaddr.sin_addr.s_addr = htons(INADDR_ANY);
    int b = bind(s, (struct sockaddr *)&sockaddr, sizeof(sockaddr));
    if (b < 0) {
        printf("Bind socket fail, code %d %s", errno, strerror(errno));
        exit(errno);
    }
    dbg("Bind socket success")

    signal(SIGINT, signalHandler);
    signal(SIGTERM, signalHandler);
    ddbg("Accept signal with function signalHandler")

    dbg("Start select loop")
    fd_set readFdSet;
    FD_ZERO(&readFdSet);
    FD_SET(s, &readFdSet);

    char buf[BUFFER_SIZE];

    struct sockaddr_in dnsServerAddr;
    memset(&dnsServerAddr, 0, sizeof(dnsServerAddr));
    dnsServerAddr.sin_family = AF_INET;
    int hostTransRes = inet_pton(AF_INET, dnsServerIp, &dnsServerAddr.sin_addr.s_addr);
    if (hostTransRes != 1) {
        printf("Bad dnsServerIp %s", dnsServerIp);
        exit(hostTransRes);
    }
    dnsServerAddr.sin_port = htons(53);
    dbg("Create dns server addr success")

    HashTable* idTable = newHashTable();
    if (idTable == NULL) {
        printf("Create id table fail");
        exit(-1);
    }

    for (; !stop; usleep(500 * 1000)) {
        fd_set readyFdSet = readFdSet;
        dbg("Server waiting socket count %d", FD_SETSIZE)
        int selectRes = select(FD_SETSIZE, &readyFdSet, (fd_set *)NULL, (fd_set *)NULL, (struct timeval *) NULL);
        if (selectRes < 1) {
            dbg("Select socket fail, code %d %s", errno, strerror(errno));
            continue;
        }
        dbg("Await from select")
        if(FD_ISSET(s, &readyFdSet)) {
            ddbg("Receive from port 53")
            memset(buf, 0, BUFFER_SIZE);
            struct sockaddr_in sockaddrIn;
            socklen_t len = sizeof(sockaddrIn);
            long r = recvfrom(s, (void *)buf, BUFFER_SIZE, 0, (struct sockaddr *)&sockaddrIn, &len);
            if(r >= BUFFER_SIZE) {
                dbg("Buffer is to small")
            }
            if (r < 0) {
                dbg("Receive packet fail, code %d %s", errno, strerror(errno))
                continue;
            }
            char addr[16] = {0};
            inet_ntop(AF_INET, &sockaddrIn.sin_addr.s_addr, addr, INET_ADDRSTRLEN);
            ddbg("Decoder to dns packet")
            struct DnsPacket* packet = decoder(buf);
            ddbg("Receive packet from %s:%d %c", addr,  ntohs(sockaddrIn.sin_port), getQR(packet))

            if ('Q' == getQR(packet)) {
                uint16_t id = random();
                char* key = malloc(32 * sizeof(char));
                memset(key, 0, 32);
                sprintf(key, "%d", id);
                ClientId* value = malloc(sizeof(ClientId));
                value->id = packet->header.id;
                value->ip = sockaddrIn.sin_addr.s_addr;
                value->port = sockaddrIn.sin_port;
                value->deadline = time(NULL) + 60;
                putHashTable(idTable, key, value, freeClientId);
                ddbg("Random id %d", id)
                setPacketId(buf, htons(id));
                long sendRes = sendto(s, buf, r, 0, (struct sockaddr *)&dnsServerAddr, sizeof(dnsServerAddr));
                if (sendRes < 0) {
                    dbg("Send to server fail, code %d %s", errno, strerror(errno))
                    rmHashTable(idTable, key);
                }
            } else {
                uint16_t id = packet->header.id;
                char key[32];
                memset(key, 0, 32);
                sprintf(key, "%d", id);
                ddbg("Receive id %d", id)
                ClientId* value = (ClientId *) getHashTable(idTable, key);
                if (value == NULL) {
                    dbg("Ignore R packet with no client")
                    continue;
                }
                ddbg("Get value %d", value->id)
                struct sockaddr_in clientAddr;
                memset(&clientAddr, 0, sizeof(clientAddr));
                clientAddr.sin_family = AF_INET;
                clientAddr.sin_addr.s_addr = value->ip;
                clientAddr.sin_port = value->port;
                dbg("Create client addr success")
                setPacketId(buf, htons(value->id));
                long sendRes = sendto(s, buf, r, 0, (struct sockaddr *)&clientAddr, sizeof(clientAddr));
                if (sendRes < 0) {
                    dbg("Send to client fail, code %d %s", errno, strerror(errno))
                }
                rmHashTable(idTable, key);
            }

            free(packet);
        }
    }

    freeHashTable(idTable);
    ddbg("Free id Table");
}


void signalHandler(int _) {
    printf("Wait a few second to clear\n");
    stop = 1;
}
