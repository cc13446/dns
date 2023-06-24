//
// Created by cc on 23-6-24.
//

#ifndef DNS_HASHMAP_H
#define DNS_HASHMAP_H

struct kv {
    struct kv* next;
    char* key;
    void* value;
    void(*free_value)(void*);
};

/* HashTable */
typedef struct HashTable {
    struct kv ** table;
} HashTable;

HashTable* newHashTable();

void freeHashTable(HashTable* ht);

int putHashTable(HashTable* ht, char* key, void* value, void(*free_value)(void*));

void* getHashTable(HashTable* ht, char* key);

void rmHashTable(HashTable* ht, char* key);

#endif //DNS_HASHMAP_H
