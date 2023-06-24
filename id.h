//
// Created by cc on 23-6-24.
//

#ifndef DNS_ID_H
#define DNS_ID_H
#include <stdint.h>

typedef struct ClientId {
    uint32_t ip;
    uint16_t port;
    uint16_t id;
    time_t deadline;
} ClientId;

void freeClientId(void* id);
int timeoutClientId(void* id);

#endif //DNS_ID_H
