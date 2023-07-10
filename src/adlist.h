/* adlist.h - A generic doubly linked list implementation
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

#ifndef __ADLIST_H__
#define __ADLIST_H__

/* Node, List, and Iterator are the only data structures used currently. */

/**
 * 链表节点
 */
typedef struct listNode {
    // 前驱节点
    struct listNode *prev;
    // 后继节点
    struct listNode *next;
    // 节点存储的值
    void *value;
} listNode;

/**
 * 迭代器
 */
typedef struct listIter {
    // 指向具体的节点
    listNode *next;
    // 迭代的顺序
    int direction;
} listIter;

/**
 * 链表，由listNode组成
 * 双端链表
 * 无环
 */
typedef struct list {
    // 头节点
    listNode *head;
    // 尾节点
    listNode *tail;
    /**
     * dum、free、match用于实现多态链表所需的类型特定函数
     * 使用这三个属性可以为节点设置类型特定函数，所以链表可以
     * 保存各种不同类型的值
     */
    // 节点值复制函数，复制链表节点保存的值
    void *(*dup)(void *ptr);
    // 节点值释放函数，释放链表节点保存的值
    void (*free)(void *ptr);
    // 节点值对比函数，比较链表节点保存的值和另一个输入值是否相等
    int (*match)(void *ptr, void *key);
    // 链表包含的节点数量
    unsigned long len;
} list;

/* Functions implemented as macros */
// 宏定义：获取列表长度
#define listLength(l) ((l)->len)
// 宏定义：获取列表头节点
#define listFirst(l) ((l)->head)
// 宏定义：获取列表尾节点
#define listLast(l) ((l)->tail)
// 宏定义：获取当前节点的前一个节点
#define listPrevNode(n) ((n)->prev)
// 宏定义：获取当前节点的后一个节点
#define listNextNode(n) ((n)->next)
// 宏定义：获取当前节点的数据
#define listNodeValue(n) ((n)->value)

// 宏定义：设置链表的节点复制函数
#define listSetDupMethod(l,m) ((l)->dup = (m))
// 宏定义：设置链表的节点数据释放函数
#define listSetFreeMethod(l,m) ((l)->free = (m))
// 宏定义：设置链表节点的比较函数
#define listSetMatchMethod(l,m) ((l)->match = (m))

// 宏定义：获取链表节点的复制函数
#define listGetDupMethod(l) ((l)->dup)
// 宏定义：获取链表节点的数据释放函数
#define listGetFree(l) ((l)->free)
// 宏定义：获取链表节点的比较函数
#define listGetMatchMethod(l) ((l)->match)

/* Prototypes */
/**
 * 创建一个空的链表
 * @return
 */
list *listCreate(void);

/**
 * 释放链表
 * @param list
 */
void listRelease(list *list);

/**
 * 清空链表
 * @param list
 */
void listEmpty(list *list);

/**
 * 添加头节点
 * @param list
 * @param value
 * @return
 */
list *listAddNodeHead(list *list, void *value);

/**
 * 添加尾节点
 * @param list
 * @param value
 * @return
 */
list *listAddNodeTail(list *list, void *value);

/**
 * 插入节点
 * @param list
 * @param old_node
 * @param value
 * @param after
 * @return
 */
list *listInsertNode(list *list, listNode *old_node, void *value, int after);

/**
 * 删除指定节点
 * @param list
 * @param node
 */
void listDelNode(list *list, listNode *node);

/**
 * 获取指定方向的迭代器
 * @param list
 * @param direction
 * @return
 */
listIter *listGetIterator(list *list, int direction);

/**
 * 使用迭代器获取下一个节点
 * @param iter
 * @return
 */
listNode *listNext(listIter *iter);

/**
 * 释放迭代器
 * @param iter
 */
void listReleaseIterator(listIter *iter);

/**
 * 复制链表
 * @param orig
 * @return
 */
list *listDup(list *orig);

/**
 * 在链表中查找指定key对应的节点
 * @param list
 * @param key
 * @return
 */
listNode *listSearchKey(list *list, void *key);

/**
 * 获取指定索引位置处的节点
 * @param list
 * @param index
 * @return
 */
listNode *listIndex(list *list, long index);

/**
 * 使迭代器的当前位置回到链表头
 * @param list
 * @param li
 */
void listRewind(list *list, listIter *li);

/**
 * 使迭代器的当前位置回到链表尾
 * @param list
 * @param li
 */
void listRewindTail(list *list, listIter *li);

/**
 * 将尾节点移除掉，并将尾节点设置为头节点
 * @param list
 */
void listRotateTailToHead(list *list);

/**
 * 将头节点移除掉，并将头节点设置为尾节点
 * @param list
 */
void listRotateHeadToTail(list *list);

/**
 * 将链表o的元素添加到链表l的尾部
 * @param l
 * @param o
 */
void listJoin(list *l, list *o);

/* Directions for iterators */
// 迭代的方向，从头到尾
#define AL_START_HEAD 0
// 迭代的方向，从尾到头
#define AL_START_TAIL 1

#endif /* __ADLIST_H__ */
