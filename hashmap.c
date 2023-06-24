//
// Created by cc on 23-6-24.
//

#define TABLE_SIZE (1000)

#include <stddef.h>
#include <stdlib.h>
#include <memory.h>

#include "hashmap.h"

static void initKV(struct kv* kv) {
    kv->next = NULL;
    kv->key = NULL;
    kv->value = NULL;
    kv->freeValue = NULL;
}

static void freeKV(struct kv* kv) {
    if (kv) {
        if (kv->freeValue) {
            kv->freeValue(kv->value);
        }
        free(kv->key);
        kv->key = NULL;
        free(kv);
    }
}
static unsigned int hash33(char* key) {
    unsigned int hash = 0;
    while (*key) {
        hash = (hash << 5) + hash + *key++;
    }
    return hash;
}

HashTable* newHashTable()
{
    HashTable* ht = malloc(sizeof(HashTable));
    if (NULL == ht) {
        freeHashTable(ht);
        return NULL;
    }
    ht->table = malloc(sizeof(struct kv*) * TABLE_SIZE);
    if (NULL == ht->table) {
        freeHashTable(ht);
        return NULL;
    }
    memset(ht->table, 0, sizeof(struct kv*) * TABLE_SIZE);

    return ht;
}
void freeHashTable(HashTable* ht)
{
    if (ht) {
        if (ht->table) {
            for (int i = 0; i < TABLE_SIZE; i++) {
                struct kv* p = ht->table[i];
                struct kv* q = NULL;
                while (p) {
                    q = p->next;
                    freeKV(p);
                    p = q;
                }
            }
            free(ht->table);
            ht->table = NULL;
        }
        free(ht);
    }
}

int putHashTable(HashTable* ht, char* key, void* value, void(*freeValue)(void*))
{
    unsigned int i = hash33(key) % TABLE_SIZE;
    struct kv* p = ht->table[i];
    struct kv* prep = p;

    while (p) { /* if key is already stored, update its value */
        if (strcmp(p->key, key) == 0) {
            if (p->freeValue) {
                p->freeValue(p->value);
            }
            p->value = value;
            p->freeValue = freeValue;
            break;
        }
        prep = p;
        p = p->next;
    }

    if (p == NULL) {/* if key has not been stored, then add it */
        char* keyStr = malloc(strlen(key) + 1);
        if (keyStr == NULL) {
            return -1;
        }
        struct kv * kv = malloc(sizeof(struct kv));
        if (NULL == kv) {
            free(keyStr);
            keyStr = NULL;
            return -1;
        }
        initKV(kv);
        kv->next = NULL;
        strcpy(keyStr, key);
        kv->key = keyStr;
        kv->value = value;
        kv->freeValue = freeValue;

        if (prep == NULL) {
            ht->table[i] = kv;
        }
        else {
            prep->next = kv;
        }
    }
    return 0;
}

void* getHashTable(HashTable* ht, char* key) {
    unsigned int i = hash33(key) % TABLE_SIZE;
    struct kv* p = ht->table[i];
    while (p) {
        if (strcmp(key, p->key) == 0) {
            return p->value;
        }
        p = p->next;
    }
    return NULL;
}

void rmHashTable(HashTable* ht, char* key) {
    unsigned int i = hash33(key) % TABLE_SIZE;

    struct kv* p = ht->table[i];
    struct kv* prep = p;
    while (p) {
        if (strcmp(key, p->key) == 0) {
            if (p == prep) {
                ht->table[i] = NULL;
            } else {
                prep->next = p->next;
            }
            struct kv * temp = p->next;
            freeKV(p);
            p = temp;
        } else {
            prep = p;
            p = p->next;
        }
    }
}

int rmHashTableWithCondition(HashTable* ht, int(*rmCondition)(void*), long batch) {
    int res = 0;
    batch = batch > TABLE_SIZE ? TABLE_SIZE : batch;
    long rand = random() % (TABLE_SIZE - batch);
    for (long i = rand; i < rand + batch && i < TABLE_SIZE; i++) {
        struct kv* p = ht->table[i];
        struct kv* prep = p;
        while (p) {
            if (rmCondition(p->value) != 0) {
                if (p == prep) {
                    ht->table[i] = NULL;
                } else {
                    prep->next = p->next;
                }
                struct kv * temp = p->next;
                freeKV(p);
                res++;
                p = temp;
            } else {
                prep = p;
                p = p->next;
            }
        }
    }
    return res;
}