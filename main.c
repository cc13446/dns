#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <signal.h>
#include <errno.h>
#include <unistd.h>
#include <arpa/inet.h>

#include "main.h"
#include "debug.h"
#include "config.h"
#include "file.h"
#include "packet.h"

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
            ddbg("Receive packet from %s:%d Data:%s", addr,  ntohs(sockaddrIn.sin_port), buf)
        }
    }
}


void signalHandler(int _) {
    printf("Wait a few second to clear\n");
    stop = 1;
}
