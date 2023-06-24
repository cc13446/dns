//
// Created by cc on 23-6-24.
//
#include <stdlib.h>
#include <time.h>

#include "id.h"

void freeClientId(void* id) {
    if (id) {
        free(id);
    }
}

int timeoutClientId(void* id) {
    ClientId* clientId = (ClientId*) id;
    time_t t = time(NULL);
    if (clientId->deadline < t) {
        return 1;
    }
    return 0;
}
