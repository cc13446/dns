#include <stdio.h>
#include <string.h>
#include <stdlib.h>

#include "main.h"
#include "debug.h"
#include "config.h"
#include "file.h"

char* dnsServerIp = "192.168.1.1";
char* hijackFilepath = "./dns.conf";

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
    dbg("Config is ready")
    doDns();
    return 0;
}

void doDns() {
    char* hijackFileContent = readAllContent(hijackFilepath);
    dbg("Read from hijackFilePath : \n%s", hijackFileContent)

    free(hijackFileContent);
    ddbg("free hijackFileContent")
}
