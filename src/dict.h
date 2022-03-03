/* Hash Tables Implementation.
 *
 * This file implements in-memory hash tables with insert/del/replace/find/
 * get-random-element operations. Hash tables will auto-resize if needed
 * tables of power of two in size are used, collisions are handled by
 * chaining. See the source code for more information... :)
 *
 * Copyright (c) 2006-2012, Salvatore Sanfilippo <antirez at gmail dot com>
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are met:
 *
 *   * Redistributions of source code must retain the above copyright notice,
 *     this list of conditions and the following disclaimer.
 *   * Redistributions in binary form must reproduce the above copyright
 *     notice, this list of conditions and the following disclaimer in the
 *     documentation and/or other materials provided with the distribution.
 *   * Neither the name of Redis nor the names of its contributors may be used
 *     to endorse or promote products derived from this software without
 *     specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
 * AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT OWNER OR CONTRIBUTORS BE
 * LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 * CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
 * SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 * INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
 * CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 * ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 * POSSIBILITY OF SUCH DAMAGE.
 */

#include <stdint.h>

#ifndef __DICT_H
#define __DICT_H

#define DICT_OK 0
#define DICT_ERR 1

/* Unused arguments generate annoying warnings... */
#define DICT_NOTUSED(V) ((void) V)

/**
 * 哈希表的节点
 */
typedef struct dictEntry {
    // 键
    void *key;
    // 值
    union {
        void *val;
        uint64_t u64;
        int64_t s64;
        double d;
    } v;
    // 指向下一个哈希表节点的指针，用来解决冲突（链地址法），头插法
    struct dictEntry *next;
} dictEntry;

/**
 * 字典的特定类型函数
 */
typedef struct dictType {
    // 计算哈希值的函数
    uint64_t (*hashFunction)(const void *key);
    // 复制键的函数
    void *(*keyDup)(void *privdata, const void *key);
    // 复制值的函数
    void *(*valDup)(void *privdata, const void *obj);
    // 对比键的函数
    int (*keyCompare)(void *privdata, const void *key1, const void *key2);
    // 键销毁的函数
    void (*keyDestructor)(void *privdata, void *key);
    // 值销毁的函数
    void (*valDestructor)(void *privdata, void *obj);
} dictType;

/* This is our hash table structure. Every dictionary has two of this as we
 * implement incremental rehashing, for the old to the new table. */
/**
 * 哈希表
 */
typedef struct dictht {
    // 哈希表数组，每个dictEntry都保存着一个键值对
    dictEntry **table;
    // 记录了哈希表的大小
    unsigned long size;
    // 掩码，值总是等于size-1，和哈希值一起决定一个键应该被放到table数组的哪个索引上
    unsigned long sizemask;
    // 该哈希表已有节点的数量，包含next单链表的数据
    unsigned long used;
} dictht;

/**
 * 字典
 * 字典使用哈希表dictht作为底层实现
 */
typedef struct dict {
    /**
     * 类型特定函数
     * 每个dictType结构保存了一些用于操作特定类型
     * 键值对的函数，可给字典设置不同类型的特定函数
     */
    dictType *type;
    /**
     * privdata属性保存了需要传给类型特定函数的可选参数
     */
    void *privdata;
    /**
     * 字典底层实现是哈希表dictht
     * 包含两个项的数组，一般字典只使用ht[0]，
     * ht[1]会在rehash的时候使用
     */
    dictht ht[2];
    /**
     * 记录了rehash的进度，默认-1，表示没有在进行rehash；不为-1表示正在进行rehash，
     * rehashidx的值表示ht[0]的rehash操作进行到了哪个索引值
     */
    long rehashidx; /* rehashing not in progress if rehashidx == -1 */
    // 当前运行的迭代器数目
    unsigned long iterators; /* number of iterators currently running */
} dict;

/* If safe is set to 1 this is a safe iterator, that means, you can call
 * dictAdd, dictFind, and other functions against the dictionary even while
 * iterating. Otherwise it is a non safe iterator, and only dictNext()
 * should be called while iterating. */
/**
 * 迭代器
 */
typedef struct dictIterator {
    // 迭代的字典
    dict *d;
    // 当前迭代到的索引
    long index;
    // table是当前正在迭代的哈希表，safe表示当前创建的是否为安全迭代器
    int table, safe;
    // entry表示当前节点，nextEntry表示当前节点的链表下一个节点
    dictEntry *entry, *nextEntry;
    /* unsafe iterator fingerprint for misuse detection. */
    // 字典的指纹，字典未发生变化的时候，该值不会变化
    long long fingerprint;
} dictIterator;

typedef void (dictScanFunction)(void *privdata, const dictEntry *de);
typedef void (dictScanBucketFunction)(void *privdata, dictEntry **bucketref);

/* This is the initial size of every hash table */
#define DICT_HT_INITIAL_SIZE     4

/* ------------------------------- Macros ------------------------------------*/
#define dictFreeVal(d, entry) \
    if ((d)->type->valDestructor) \
        (d)->type->valDestructor((d)->privdata, (entry)->v.val)

#define dictSetVal(d, entry, _val_) do { \
    if ((d)->type->valDup) \
        (entry)->v.val = (d)->type->valDup((d)->privdata, _val_); \
    else \
        (entry)->v.val = (_val_); \
} while(0)

#define dictSetSignedIntegerVal(entry, _val_) \
    do { (entry)->v.s64 = _val_; } while(0)

#define dictSetUnsignedIntegerVal(entry, _val_) \
    do { (entry)->v.u64 = _val_; } while(0)

#define dictSetDoubleVal(entry, _val_) \
    do { (entry)->v.d = _val_; } while(0)

#define dictFreeKey(d, entry) \
    if ((d)->type->keyDestructor) \
        (d)->type->keyDestructor((d)->privdata, (entry)->key)

#define dictSetKey(d, entry, _key_) do { \
    if ((d)->type->keyDup) \
        (entry)->key = (d)->type->keyDup((d)->privdata, _key_); \
    else \
        (entry)->key = (_key_); \
} while(0)

#define dictCompareKeys(d, key1, key2) \
    (((d)->type->keyCompare) ? \
        (d)->type->keyCompare((d)->privdata, key1, key2) : \
        (key1) == (key2))

/**
 * 获取键的哈希值
 */
#define dictHashKey(d, key) (d)->type->hashFunction(key)
#define dictGetKey(he) ((he)->key)
#define dictGetVal(he) ((he)->v.val)
#define dictGetSignedIntegerVal(he) ((he)->v.s64)
#define dictGetUnsignedIntegerVal(he) ((he)->v.u64)
#define dictGetDoubleVal(he) ((he)->v.d)
#define dictSlots(d) ((d)->ht[0].size+(d)->ht[1].size)
#define dictSize(d) ((d)->ht[0].used+(d)->ht[1].used)
/**
 * 字典是否正在rehash，直接判断rehashidx是否等于-1
 */
#define dictIsRehashing(d) ((d)->rehashidx != -1)

/* API */
/**
 * 创建字典
 * @param type
 * @param privDataPtr
 * @return
 */
dict *dictCreate(dictType *type, void *privDataPtr);
/**
 * 字典扩容
 * @param d
 * @param size
 * @return
 */
int dictExpand(dict *d, unsigned long size);
/**
 * 添加键值对
 * @param d
 * @param key
 * @param val
 * @return
 */
int dictAdd(dict *d, void *key, void *val);
/**
 * 添加键
 * @param d
 * @param key
 * @param existing
 * @return
 */
dictEntry *dictAddRaw(dict *d, void *key, dictEntry **existing);
dictEntry *dictAddOrFind(dict *d, void *key);
int dictReplace(dict *d, void *key, void *val);
/**
 * 删除元素
 * @param d
 * @param key
 * @return
 */
int dictDelete(dict *d, const void *key);
dictEntry *dictUnlink(dict *ht, const void *key);
void dictFreeUnlinkedEntry(dict *d, dictEntry *he);
void dictRelease(dict *d);
/**
 * 根据键查询元素
 * @param d
 * @param key
 * @return
 */
dictEntry * dictFind(dict *d, const void *key);
void *dictFetchValue(dict *d, const void *key);
int dictResize(dict *d);
/**
 * 初始化迭代器，普通迭代器只遍历数据
 * @param d
 * @return
 */
dictIterator *dictGetIterator(dict *d);
/**
 * 初始化安全的迭代器，安全迭代器遍历的同时可删除数据
 * @param d
 * @return
 */
dictIterator *dictGetSafeIterator(dict *d);
/**
 * 通过迭代器获取下一个节点
 * @param iter
 * @return
 */
dictEntry *dictNext(dictIterator *iter);
/**
 * 释放迭代器
 * @param iter
 */
void dictReleaseIterator(dictIterator *iter);
dictEntry *dictGetRandomKey(dict *d);
unsigned int dictGetSomeKeys(dict *d, dictEntry **des, unsigned int count);
void dictGetStats(char *buf, size_t bufsize, dict *d);
uint64_t dictGenHashFunction(const void *key, int len);
uint64_t dictGenCaseHashFunction(const unsigned char *buf, int len);
void dictEmpty(dict *d, void(callback)(void*));
void dictEnableResize(void);
void dictDisableResize(void);
/**
 * rehash操作
 * @param d
 * @param n
 * @return
 */
int dictRehash(dict *d, int n);
int dictRehashMilliseconds(dict *d, int ms);
void dictSetHashFunctionSeed(uint8_t *seed);
uint8_t *dictGetHashFunctionSeed(void);
unsigned long dictScan(dict *d, unsigned long v, dictScanFunction *fn, dictScanBucketFunction *bucketfn, void *privdata);
uint64_t dictGetHash(dict *d, const void *key);
dictEntry **dictFindEntryRefByPtrAndHash(dict *d, const void *oldptr, uint64_t hash);

/* Hash table types */
extern dictType dictTypeHeapStringCopyKey;
extern dictType dictTypeHeapStrings;
extern dictType dictTypeHeapStringCopyKeyValue;

#endif /* __DICT_H */
