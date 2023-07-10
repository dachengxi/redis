/* adlist.c - A generic doubly linked list implementation
 *
 * Copyright (c) 2006-2010, Salvatore Sanfilippo <antirez at gmail dot com>
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


#include <stdlib.h>
#include "adlist.h"
#include "zmalloc.h"

/*
 * A generic doubly linked list
 * 双端链表
 */

/* Create a new list. The created list can be freed with
 * AlFreeList(), but private value of every node need to be freed
 * by the user before to call AlFreeList().
 *
 * On error, NULL is returned. Otherwise the pointer to the new list. */
/**
 * 创建一个空的链表
 * @return
 */
list *listCreate(void)
{

    // 声明链表结构体
    struct list *list;

    // 为链表结构体分配内存
    if ((list = zmalloc(sizeof(*list))) == NULL)
        return NULL;
    // 初始化头尾节点为NULL
    list->head = list->tail = NULL;
    // 初始化链表长度为0
    list->len = 0;
    // 初始化链表复制函数为NULL
    list->dup = NULL;
    // 初始化节点数据释放函数为NULL
    list->free = NULL;
    // 初始化节点比较函数为NULL
    list->match = NULL;
    return list;
}

/* Remove all the elements from the list without destroying the list itself. */
/**
 * 清空链表
 * @param list
 */
void listEmpty(list *list)
{
    // 链表长度
    unsigned long len;
    // 当前节点和下一个节点
    listNode *current, *next;

    // 从头节点开始
    current = list->head;
    // 链表长度
    len = list->len;
    // 从头遍历链表
    while(len--) {
        // 保存当前节点的下一个节点
        next = current->next;
        // 使用节点释放函数释放当前节点的数据
        if (list->free) list->free(current->value);
        // 释放当前节点
        zfree(current);
        // 当前节点指向下一个节点
        current = next;
    }
    // 头尾节点设置为NULL
    list->head = list->tail = NULL;
    // 列表长度置为0
    list->len = 0;
}

/* Free the whole list.
 *
 * This function can't fail. */
/**
 * 释放整个链表
 * @param list
 */
void listRelease(list *list)
{
    // 先清空链表中的节点
    listEmpty(list);
    // 释放链表本身
    zfree(list);
}

/* Add a new node to the list, to head, containing the specified 'value'
 * pointer as value.
 *
 * On error, NULL is returned and no operation is performed (i.e. the
 * list remains unaltered).
 * On success the 'list' pointer you pass to the function is returned. */
/**
 * 将指定的值添加到链表头
 * @param list
 * @param value
 * @return
 */
list *listAddNodeHead(list *list, void *value)
{
    // 声明链表节点的结构体，新节点
    listNode *node;

    // 为链表节点分配内存
    if ((node = zmalloc(sizeof(*node))) == NULL)
        return NULL;
    // 设置新的节点的值
    node->value = value;
    // 当前链表中还没有节点
    if (list->len == 0) {
        // 头尾节点都指向新的节点
        list->head = list->tail = node;
        // 新节点的前驱后和后继节点都是NULL
        node->prev = node->next = NULL;
    }
    // 当前链表中已经有节点
    else {
        // 新节点的前驱节点设置为NULL
        node->prev = NULL;
        // 新节点的后继节点指向链表的第一个结点
        node->next = list->head;
        // 链表的第一个节点的前驱节点指向新节点
        list->head->prev = node;
        // 链表的头结点指向新节点
        list->head = node;
    }
    // 链表长度加1
    list->len++;
    return list;
}

/* Add a new node to the list, to tail, containing the specified 'value'
 * pointer as value.
 *
 * On error, NULL is returned and no operation is performed (i.e. the
 * list remains unaltered).
 * On success the 'list' pointer you pass to the function is returned. */
/**
 * 在链表尾部添加节点
 * @param list
 * @param value
 * @return
 */
list *listAddNodeTail(list *list, void *value)
{
    // 声明新节点结构体
    listNode *node;

    // 为新节点分配内存
    if ((node = zmalloc(sizeof(*node))) == NULL)
        return NULL;
    // 设置新节点的值
    node->value = value;
    // 链表中还没有节点
    if (list->len == 0) {
        // 头和尾节点都指向新节点
        list->head = list->tail = node;
        // 新节点的前驱和后继都指向NULL
        node->prev = node->next = NULL;
    }
    // 链表中已经有节点
    else {
        // 新节点的前驱指向尾部的节点
        node->prev = list->tail;
        // 新节点的后继节点指向NULL
        node->next = NULL;
        // 尾部节点的后继节点指向新节点
        list->tail->next = node;
        // 尾节点指向新节点
        list->tail = node;
    }
    // 链表长度加1
    list->len++;
    return list;
}

/**
 * 在指定的节点的位置前面或者后面插入节点
 * @param list
 * @param old_node
 * @param value
 * @param after
 * @return
 */
list *listInsertNode(list *list, listNode *old_node, void *value, int after) {
    // 声明新节点结构体
    listNode *node;

    // 为新节点结构体分配内存
    if ((node = zmalloc(sizeof(*node))) == NULL)
        return NULL;
    // 为新节点设置值
    node->value = value;
    // 在指定节点的后面插入节点
    if (after) {
        // 新节点的前驱节点指向指定的节点
        node->prev = old_node;
        // 新节点的后继节点指向指定的节点的后继节点
        node->next = old_node->next;
        // 如果正好在尾部插入
        if (list->tail == old_node) {
            // 尾节点指向新节点
            list->tail = node;
        }
    }
    // 在指定节点的前面插入节点
    else {
        // 新节点的后继节点指向指定的节点
        node->next = old_node;
        // 新节点的前驱节点指向指定的节点的前驱节点
        node->prev = old_node->prev;
        // 正好在头部插入
        if (list->head == old_node) {
            // 头节点指向新节点
            list->head = node;
        }
    }
    // 新节点有前驱节点
    if (node->prev != NULL) {
        // 将前驱节点的后继节点指向新节点
        node->prev->next = node;
    }
    // 新节点有后继节点
    if (node->next != NULL) {
        // 将后继节点的前驱指向新节点
        node->next->prev = node;
    }
    // 链表长度加1
    list->len++;
    return list;
}

/* Remove the specified node from the specified list.
 * It's up to the caller to free the private value of the node.
 *
 * This function can't fail. */
/**
 * 删除指定的节点
 * @param list
 * @param node
 */
void listDelNode(list *list, listNode *node)
{
    // 要删除的节点有前驱节点
    if (node->prev)
        // 要删除的节点的前驱节点的后继指向要删除的节点的后继
        node->prev->next = node->next;
    // 要删除的节点没有前驱节点，说明是第一个节点
    else
        // 将头节点指向要删除节点的后继节点
        list->head = node->next;
    // 要删除的节点有后继节点
    if (node->next)
        // 要删除的节点的后继节点的前驱设置为要删除的节点的前驱
        node->next->prev = node->prev;
    // 要删除的节点没有后继节点，说明是最后一个节点
    else
        // 尾节点指向要删除节点的前驱节点
        list->tail = node->prev;
    // 使用释放函数，释放要删除节点的值
    if (list->free) list->free(node->value);
    // 将节点本身释放掉
    zfree(node);
    // 链表长度减1
    list->len--;
}

/* Returns a list iterator 'iter'. After the initialization every
 * call to listNext() will return the next element of the list.
 *
 * This function can't fail. */
/**
 * 获取链表的迭代器
 * @param list
 * @param direction
 * @return
 */
listIter *listGetIterator(list *list, int direction)
{
    // 声明迭代器的结构体
    listIter *iter;

    // 给迭代器的结构体分配内存
    if ((iter = zmalloc(sizeof(*iter))) == NULL) return NULL;
    // 从头开始遍历的迭代器
    if (direction == AL_START_HEAD)
        // 将迭代器的next指向头结点
        iter->next = list->head;
    // 从尾开始遍历的迭代器
    else
        // 将迭代器的next指向尾节点
        iter->next = list->tail;
    // 设置迭代器的方向
    iter->direction = direction;
    return iter;
}

/* Release the iterator memory */
/**
 * 释放迭代器
 * @param iter
 */
void listReleaseIterator(listIter *iter) {
    zfree(iter);
}

/* Create an iterator in the list private iterator structure */
/**
 * 使迭代器的当前位置回到链表头
 * @param list
 * @param li
 */
void listRewind(list *list, listIter *li) {
    // 迭代器的next指向链表头节点
    li->next = list->head;
    // 方向是从头开始遍历
    li->direction = AL_START_HEAD;
}

/**
 * 使迭代器的当前位置回到链表尾
 * @param list
 * @param li
 */
void listRewindTail(list *list, listIter *li) {
    // 迭代器的next指向链表尾节点
    li->next = list->tail;
    // 方向是从尾部开始遍历
    li->direction = AL_START_TAIL;
}

/* Return the next element of an iterator.
 * It's valid to remove the currently returned element using
 * listDelNode(), but not to remove other elements.
 *
 * The function returns a pointer to the next element of the list,
 * or NULL if there are no more elements, so the classical usage patter
 * is:
 *
 * iter = listGetIterator(list,<direction>);
 * while ((node = listNext(iter)) != NULL) {
 *     doSomethingWith(listNodeValue(node));
 * }
 *
 * */
/**
 * 使用迭代器获取下一个节点
 * @param iter
 * @return
 */
listNode *listNext(listIter *iter)
{
    // 下一个节点
    listNode *current = iter->next;

    // 下一个节点不为NULL
    if (current != NULL) {
        // 如果是从头开始遍历的
        if (iter->direction == AL_START_HEAD)
            // 将迭代的next指向下一个节点的下一个节点
            iter->next = current->next;
        // 如果是从尾部开始遍历的
        else
            // 将迭代器的next指向下一个节点的上一个节点
            iter->next = current->prev;
    }
    return current;
}

/* Duplicate the whole list. On out of memory NULL is returned.
 * On success a copy of the original list is returned.
 *
 * The 'Dup' method set with listSetDupMethod() function is used
 * to copy the node value. Otherwise the same pointer value of
 * the original node is used as value of the copied node.
 *
 * The original list both on success or error is never modified. */
/**
 * 复制整个链表
 * @param orig
 * @return
 */
list *listDup(list *orig)
{
    // 声明新的链表结构体
    list *copy;
    // 迭代器
    listIter iter;
    // 当前节点
    listNode *node;

    // 创建新的链表
    if ((copy = listCreate()) == NULL)
        return NULL;
    // 复制函数
    copy->dup = orig->dup;
    // 释放函数
    copy->free = orig->free;
    // 比较函数
    copy->match = orig->match;
    // 迭代器的位置指向链表头，需要从头开始遍历
    listRewind(orig, &iter);
    // 遍历原始链表
    while((node = listNext(&iter)) != NULL) {
        void *value;

        // 如果有复制函数，则使用复制函数进行复制
        if (copy->dup) {
            // 复制节点的数据
            value = copy->dup(node->value);
            // 数据为NULL
            if (value == NULL) {
                // 释放新的链表
                listRelease(copy);
                // 直接返回
                return NULL;
            }
        }
        // 没有复制函数
        else
            // 直接使用原始链表节点的数据的指针进行赋值
            value = node->value;
        // 把复制的值添加到新链表的尾部
        if (listAddNodeTail(copy, value) == NULL) {
            // 添加不成功，则释放新链表
            listRelease(copy);
            // 直接返回
            return NULL;
        }
    }
    return copy;
}

/* Search the list for a node matching a given key.
 * The match is performed using the 'match' method
 * set with listSetMatchMethod(). If no 'match' method
 * is set, the 'value' pointer of every node is directly
 * compared with the 'key' pointer.
 *
 * On success the first matching node pointer is returned
 * (search starts from head). If no matching node exists
 * NULL is returned. */
/**
 * 根据指定的key在链表中搜索节点
 * @param list
 * @param key
 * @return
 */
listNode *listSearchKey(list *list, void *key)
{
    // 迭代器
    listIter iter;
    // 当前节点
    listNode *node;

    // 使迭代器的位置指向链表头，需要从头遍历
    listRewind(list, &iter);
    // 遍历链表
    while((node = listNext(&iter)) != NULL) {
        // 如果有节点比较函数，则使用节点比较函数进行比较
        if (list->match) {
            // 如果指定的key和节点的值相同
            if (list->match(node->value, key)) {
                // 返回匹配到的节点
                return node;
            }
        }
        // 没有节点比较函数，则直接和值对比
        else {
            // 指定的key和节点的值相同
            if (key == node->value) {
                // 返回匹配到的节点
                return node;
            }
        }
    }
    // 匹配不到，则返回NULL
    return NULL;
}

/* Return the element at the specified zero-based index
 * where 0 is the head, 1 is the element next to head
 * and so on. Negative integers are used in order to count
 * from the tail, -1 is the last element, -2 the penultimate
 * and so on. If the index is out of range NULL is returned. */
/**
 * 查询指定索引位置处的节点
 * @param list
 * @param index
 * @return
 */
listNode *listIndex(list *list, long index) {
    // 存放找到的节点
    listNode *n;

    // 指定的索引小于0，则从尾部开始遍历
    if (index < 0) {
        index = (-index)-1;
        // 尾部开始遍历
        n = list->tail;
        while(index-- && n) n = n->prev;
    }
    // index大于0，从头开始遍历
    else {
        n = list->head;
        while(index-- && n) n = n->next;
    }
    return n;
}

/* Rotate the list removing the tail node and inserting it to the head. */
/**
 * 移除最后的节点，并插入到头部
 * @param list
 */
void listRotateTailToHead(list *list) {
    // 链表中节点小于等于1，无需进行操作
    if (listLength(list) <= 1) return;

    /* Detach current tail */
    // 最后的节点
    listNode *tail = list->tail;
    // 尾节点指向最后的节点的前驱节点
    list->tail = tail->prev;
    // 新的尾节点的后继指向NULL
    list->tail->next = NULL;
    /* Move it as head */
    // 头部节点的前驱指向老的尾节点
    list->head->prev = tail;
    // 老的尾节点的前驱指向NULL
    tail->prev = NULL;
    // 老的尾节点的后继节点指向原来的头部节点
    tail->next = list->head;
    // 头节点指向老的尾部的节点
    list->head = tail;
}

/* Rotate the list removing the head node and inserting it to the tail. */
/**
 * 移除头部的节点，插入到链表尾部
 * @param list
 */
void listRotateHeadToTail(list *list) {
    // 链表节点小于等于1个，不需要进行操作
    if (listLength(list) <= 1) return;

    // 老的头部节点
    listNode *head = list->head;
    /* Detach current head */
    // 头节点指向老的头部节点的后继
    list->head = head->next;
    // 头节点的前驱指向NULL
    list->head->prev = NULL;
    /* Move it as tail */
    // 原来的尾节点的后继指向老的头节点
    list->tail->next = head;
    // 老的头节点的后继节点指向NULL
    head->next = NULL;
    // 老的头节点的前驱指向原来的尾部节点
    head->prev = list->tail;
    // 尾节点指向老的头节点
    list->tail = head;
}

/* Add all the elements of the list 'o' at the end of the
 * list 'l'. The list 'other' remains empty but otherwise valid. */
/**
 * 将链表o中的元素添加到链表l的尾部
 * @param l
 * @param o
 */
void listJoin(list *l, list *o) {
    // 链表o的头结点不为NULL
    if (o->head)
        // 链表o的头结点的前驱指向链表l的尾节点
        o->head->prev = l->tail;

    // 链表l的尾节点不为NULL
    if (l->tail)
        // 链表l的尾节点的后继指向链表o的头结点
        l->tail->next = o->head;
    // 链表l的尾节点为NULL
    else
        // 链表l的头结点指向链表o的头结点
        l->head = o->head;

    // 链表o的尾节点不为NULL，则链表l的尾节点指向链表o的尾节点
    if (o->tail) l->tail = o->tail;
    // 链表l的长度需要加上链表o的长度
    l->len += o->len;

    /* Setup other as an empty list. */
    // 链表o的头尾节点指向NULL
    o->head = o->tail = NULL;
    // 链表o的长度置为0
    o->len = 0;
}
