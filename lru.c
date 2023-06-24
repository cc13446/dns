//
// Created by cc on 23-6-24.
//
#include <stddef.h>
#include <stdlib.h>
#include <stdio.h>
#include <errno.h>
#include <string.h>
#include <time.h>

#include "lru.h"
#include "debug.h"

static void freeList(LRUCache *cache);

static CacheListEntry* initCacheEntry(char *key, void *data, size_t dataLen, time_t ttl) {

    CacheListEntry* entry = NULL;
    if (NULL == (entry = malloc(sizeof(*entry)))) {
        dbg("Cache entry malloc fail, code %d %s", errno, strerror(errno))
        return NULL;
    }
    memset(entry, 0, sizeof(*entry));
    entry->key = malloc(strlen(key) + 1);
    strcpy(entry->key,key);

    entry->value = malloc(dataLen);
    entry->dataLen = dataLen;
    memcpy(entry->value, data, dataLen);
    entry->ttl = ttl;
    return entry;
}

static void freeCacheEntry(CacheListEntry* entry) {
    if (entry) {
        if (entry->key) {
            free(entry->key);
        }
        if (entry->value) {
            free(entry->value);
        }
        free(entry);
    }
}

LRUCache* initLRUCache(int capacity) {
    LRUCache* cache = NULL;
    if (NULL == (cache = malloc(sizeof(*cache)))) {
        dbg("Cache malloc fail, code %d %s", errno, strerror(errno))
        return NULL;
    }
    memset(cache, 0, sizeof(*cache));
    cache->cacheCapacity = capacity;
    cache->hashMap = malloc(sizeof(CacheListEntry*) * capacity);
    if (NULL == cache->hashMap) {
        free(cache);
        dbg("Cache hashmap malloc fail, code %d %s", errno, strerror(errno))
        return NULL;
    }
    memset(cache->hashMap, 0, sizeof(CacheListEntry*) * capacity);
    return cache;
}

void freeLRUCache(LRUCache *cache) {
    if (cache) {
        //free hashMap
        if (cache->hashMap) {
            free(cache->hashMap);
        }
        //free linklist
        freeList(cache);
        free(cache);
    }
}


static void removeFromList(LRUCache *cache, CacheListEntry *entry) {
    if (cache->cacheSize==0) {
        return;
    }

    if (entry==cache->listHead && entry==cache->listTail) {
        /* 链表中仅剩当前一个节点 */
        cache->listHead = cache->listTail = NULL;
    } else if (entry == cache->listHead) {
        /* 欲删除节点位于表头 */
        cache->listHead = entry->listNext;
        cache->listHead->listPrev = NULL;
    } else if (entry == cache->listTail) {
        /* 欲删除节点位于表尾 */
        cache->listTail = entry->listPrev;
        cache->listTail->listNext = NULL;
    } else {
        /* 其他非表头表尾情况，直接摘抄节点 */
        entry->listPrev->listNext = entry->listNext;
        entry->listNext->listPrev = entry->listPrev;
    }
    /* 删除成功， 链表节点数减1 */
    cache->cacheSize--;
}

static CacheListEntry* insertToListHead(LRUCache *cache, CacheListEntry *entry) {
    CacheListEntry *removedEntry = NULL;
    ++cache->cacheSize;

    if (cache->cacheSize > cache->cacheCapacity) {
        /* 如果缓存满了, 淘汰最久没有被访问到的缓存数据单元 */
        CacheListEntry *temp = cache->listTail;
        while(temp && temp->ttl == -1) {
            temp = temp->listPrev;
        }
        if (temp) {
            removeFromList(cache, temp);
            removedEntry = temp;
        }
    }

    if (cache->listHead == NULL && cache->listTail == NULL) {
        /*如果当前链表为空链表*/
        cache->listHead = cache->listTail = entry;
    } else {
        /*当前链表非空， 插入表头*/
        entry->listNext = cache->listHead;
        entry->listPrev = NULL;
        cache->listHead->listPrev = entry;
        cache->listHead = entry;
    }

    return removedEntry;
}

/*释放整个链表*/
static void freeList(LRUCache* cache) {
    /*链表为空*/
    if (0 == cache->cacheSize) {
        return;
    }

    CacheListEntry *entry = cache->listHead;
    /*遍历删除链表上所有节点*/
    while(entry) {
        CacheListEntry *temp = entry->listNext;
        freeCacheEntry(entry);
        entry = temp;
    }
    cache->cacheSize = 0;
}

/*辅助接口， 用于保证最近访问的节点总是位于链表表头*/
static void updateLRUList(LRUCache *cache, CacheListEntry *entry) {
    /*将节点从链表中摘抄*/
    removeFromList(cache, entry);
    /*将节点插入链表表头*/
    insertToListHead(cache, entry);
}

static unsigned int hashKey(LRUCache *cache,char* key) {
    unsigned int len = strlen(key);
    unsigned int b    = 378551;
    unsigned int a    = 63689;
    unsigned int hash = 0;
    for(int i = 0; i < len; key++, i++)
    {
        hash = hash * a + (unsigned int)(*key);
        a    = a * b;
    }
    return hash%(cache->cacheCapacity);
}

static CacheListEntry *getValueFromHashMap(LRUCache *cache, char *key) {
    /*1.使用哈希函数定位数据存放于哪个槽*/
    CacheListEntry *entry = cache->hashMap[hashKey(cache, key)];

    /*2.遍历查询槽内链表， 找到准确的数据项*/
    while (entry) {
        if (!strcmp(entry->key, key)) {
            break;
        }
        entry = entry->hashListNext;
    }

    return entry;
}

static void insertEntryToHashMap(LRUCache *cache, CacheListEntry *entry) {

    /*1.使用哈希函数定位数据存放在哪个槽*/
    CacheListEntry *n = cache->hashMap[hashKey(cache, entry->key)];
    if (n != NULL) {
        /*2.如果槽内已有其他数据项， 将槽内的数据项与当前欲加入数据项链成链表，
        当前欲加入数据项为表头*/
        entry->hashListNext = n;
        n->hashListPrev = entry;
    }
    /*3.将数据项加入数据槽内*/
    cache->hashMap[hashKey(cache, entry->key)] = entry;
}

static void removeEntryFromHashMap(LRUCache *cache, CacheListEntry *entry) {
    /*无需做任何删除操作的情况*/
    if (NULL == entry || NULL == cache || NULL == cache->hashMap) {
        return;
    }
    // 如果不是哈系表头 直接去掉就可以
    if (entry->hashListPrev) {
        entry->hashListPrev->hashListNext = entry->hashListNext;
        if (entry->hashListNext) {
            entry->hashListNext->hashListPrev = entry->hashListPrev;
        }
        return;
    }

    /*1.使用哈希函数定位数据位于哪个槽*/
    unsigned int k = hashKey(cache, entry->key);
    CacheListEntry *n = cache->hashMap[k];
    /*2.节点在表头*/
    if (n) {
        if (n->key == entry->key) {
            cache->hashMap[k] = n->hashListNext;
            if (entry->hashListNext) {
                n->hashListNext->hashListPrev = NULL;
            }
            return;
        }
    }
}


void setLRUCache(LRUCache *cache, char *key, void *data, size_t dataLen, time_t ttl) {

    if (ttl != -1 && ttl < time(NULL)) {
        return;
    }

    /*从哈希表中查找数据是否已经存在缓存中*/
    CacheListEntry *entry = getValueFromHashMap(cache, key);
    if (entry != NULL) {
        /*更新数据， 将该数据更新到链表表头*/
        void* old = entry->value;
        entry->value = malloc(dataLen);
        entry->dataLen = dataLen;
        memcpy(entry->value, data, dataLen);
        free(old);
        entry->ttl = ttl;
        updateLRUList(cache, entry);
    } else {
        /*数据没在缓存中*/
        /*新建缓存单元*/
        entry = initCacheEntry(key, data, dataLen, ttl);

        /*将新建缓存单元插入链表表头*/
        CacheListEntry *removedEntry = insertToListHead(cache, entry);
        if (NULL != removedEntry) {
            /*缓存满, 需要淘汰最久没有被访问到的缓存数据单元*/
            removeEntryFromHashMap(cache, removedEntry);
            freeCacheEntry(removedEntry);
        }
        /*将新建缓存单元插入哈希表*/
        insertEntryToHashMap(cache, entry);
    }
}

size_t getLRUCache(LRUCache* cache, char *key, void *dst) {
    CacheListEntry* entry = getValueFromHashMap(cache, key);
    if (NULL != entry) {
        // TTL 过期
        if (entry->ttl != -1 && time(NULL) > entry->ttl) {
            removeFromList(cache, entry);
            removeEntryFromHashMap(cache, entry);
            freeCacheEntry(entry);
            return 0;
        }
        /*缓存中存在该数据， 更新至链表表头*/
        updateLRUList(cache, entry);
        /*返回数据*/
        memcpy(dst, entry->value, entry->dataLen);
        return entry->dataLen;
    } else {
        /*缓存中不存在相关数据*/
        return 0;
    }
}


int clearTimeoutLRUCache(LRUCache* cache) {
    if (NULL == cache || 0 == cache->cacheSize) {
        return 0;
    }

    int res = 0;

    CacheListEntry* entry = cache->listHead;
    while (entry) {
        ddbg("Scan entry %s %ld", entry->key, entry->ttl)
        CacheListEntry* next = entry->listNext;
        if (entry->ttl != -1 && entry->ttl < time(NULL)) {
            removeFromList(cache, entry);
            removeEntryFromHashMap(cache, entry);
            freeCacheEntry(entry);
            res++;
        }
        entry = next;
    }
    return res;
}
