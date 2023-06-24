//
// Created by cc on 23-6-24.
//

#ifndef DNS_LRU_H
#define DNS_LRU_H

#include "hashmap.h"

typedef struct CacheListEntry {
    char* key;
    char* value;

    time_t ttl;

    struct CacheListEntry *hashListPrev;   /* 缓存哈希表指针， 指向哈希链表的前一个元素 */
    struct CacheListEntry *hashListNext;   /* 缓存哈希表指针， 指向哈希链表的后一个元素 */

    struct CacheListEntry *listPrev;    /* 缓存双向链表指针， 指向链表的前一个元素 */
    struct CacheListEntry *listNext;    /* 缓存双向链表指针， 指向链表后一个元素 */
}CacheListEntry;

typedef struct LRUCache {
    int cacheCapacity;
    int cacheSize;

    CacheListEntry **hashMap;

    CacheListEntry *listHead;
    CacheListEntry *listTail;
} LRUCache;

LRUCache* initLRUCache(int capacity);
void freeLRUCache(LRUCache *cache);

int setLRUCache(LRUCache *cache, char *key, char *data, time_t ttl);
char* getLRUCache(LRUCache* cache, char *key);

void printLRUCache(LRUCache* cache);

int clearTimeoutLRUCache(LRUCache* cache);

#endif //DNS_LRU_H
