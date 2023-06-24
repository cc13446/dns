//
// Created by cc on 23-6-24.
//
#include <stdlib.h>

#include "id.h"

void freeClientId(void* id) {
    if (id) {
        free(id);
    }
}
