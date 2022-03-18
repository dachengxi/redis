/* A simple event-driven programming library. Originally I wrote this code
 * for the Jim's event-loop (Jim is a Tcl interpreter) but later translated
 * it in form of a library for easy reuse.
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

#ifndef __AE_H__
#define __AE_H__

#include <time.h>

#define AE_OK 0
#define AE_ERR -1

#define AE_NONE 0       /* No events registered. */
#define AE_READABLE 1   /* Fire when descriptor is readable. */
#define AE_WRITABLE 2   /* Fire when descriptor is writable. */
#define AE_BARRIER 4    /* With WRITABLE, never fire the event if the
                           READABLE event already fired in the same event
                           loop iteration. Useful when you want to persist
                           things to disk before sending replies, and want
                           to do that in a group fashion. */

#define AE_FILE_EVENTS 1
#define AE_TIME_EVENTS 2
#define AE_ALL_EVENTS (AE_FILE_EVENTS|AE_TIME_EVENTS)
#define AE_DONT_WAIT 4
#define AE_CALL_AFTER_SLEEP 8

#define AE_NOMORE -1
#define AE_DELETED_EVENT_ID -1

/* Macros */
#define AE_NOTUSED(V) ((void) V)

struct aeEventLoop;

/* Types and data structures */
typedef void aeFileProc(struct aeEventLoop *eventLoop, int fd, void *clientData, int mask);
typedef int aeTimeProc(struct aeEventLoop *eventLoop, long long id, void *clientData);
typedef void aeEventFinalizerProc(struct aeEventLoop *eventLoop, void *clientData);
typedef void aeBeforeSleepProc(struct aeEventLoop *eventLoop);

/* File event structure
 * 对事件的封装，内部保存有监听的文件描述符、监听的事件、回调函数
 */
/**
 * 文件事件
 * redis服务器通过套接字与客户端连接，文件事件是服务器对套接字操作的抽象。
 * 服务端与客户端的通信会产生相应的文件事件，服务器通过监听并处理这些事件
 * 来完成一系列网络通信操作。
 */
typedef struct aeFileEvent {
    /**
     * 监控的文件事件类型
     * AE_READABLE可读事件、AE_WRITEABLE可写事件
     */
    int mask; /* one of AE_(READABLE|WRITABLE|BARRIER) */
    /**
     * 函数指针，指向读事件处理函数
     */
    aeFileProc *rfileProc;
    /**
     * 函数指针，指向写事件处理函数
     */
    aeFileProc *wfileProc;
    /**
     * 客户端对象
     */
    void *clientData;
} aeFileEvent;

/* Time event structure */
/**
 * 时间事件
 * redis服务器中一些操作需要在给定的时间点执行，时间事件就是服务器对这类定时操作的抽象。
 *
 * 时间事件分为两类：
 * - 定时事件：在指定的时间后执行一次
 * - 周期性事件：每个指定的时间执行一次
 *
 * 一个时间事件是定时事件还是周期性事件取决于时间事件处理器的返回值：
 * - 返回AE_NOMORE，为定时事件，该事件在达到一次之后就会被删除，之后不再到达
 * - 返回非AE_NOMORE，为周期性事件，服务器会根据事件处理器返回的值，对时间事件的when
 *   属性进行更新，让这个事件在一段时候之后再次到达。
 *
 * 所有时间事件都放在一个无序链表中，当时间事件执行器运行时，就遍历整个链表，查找所有已经
 * 到达的时间事件，并调用相应的事件处理器。
 */
typedef struct aeTimeEvent {
    /**
     * 时间事件的全局唯一ID，从小到大顺序递增
     */
    long long id; /* time event identifier. */
    /**
     * 时间事件触发的秒数
     */
    long when_sec; /* seconds */
    /**
     * 时间事件触发的毫秒数
     */
    long when_ms; /* milliseconds */
    /**
     * 函数指针，时间事件处理函数，此函数的返回值表示此时间事件下次应该被触发的时间（毫秒），
     * 是一个相对时间，从当前时间算起，过了这么多毫秒后此时间事件会被触发。
     */
    aeTimeProc *timeProc;
    /**
     * 函数指针，删除时间事件节点之前会调用此函数
     */
    aeEventFinalizerProc *finalizerProc;
    /**
     * 对应的客户端对象
     */
    void *clientData;
    /**
     * 时间事件使用链表维护，指向上一个时间事件
     */
    struct aeTimeEvent *prev;
    /**
     * 时间事件使用链表维护，指向下一个时间事件
     */
    struct aeTimeEvent *next;
} aeTimeEvent;

/* A fired event */
/**
 * 触发的事件
 */
typedef struct aeFiredEvent {
    /**
     * 发生事件的socket文件描述符
     */
    int fd;
    /**
     * 发生的事件类型，AE_READABLE可读事件、AE_WRITEABLE可写事件
     */
    int mask;
} aeFiredEvent;

/* State of an event based program */
/**
 * redis服务器是事件驱动模式，事件分为两种：
 * - 文件事件，socket的读写事件
 * - 时间事件，定时任务相关的事件
 *
 * aeEventLoop来封装事件
 */
typedef struct aeEventLoop {
    int maxfd;   /* highest file descriptor currently registered */
    int setsize; /* max number of file descriptors tracked */
    long long timeEventNextId;
    time_t lastTime;     /* Used to detect system clock skew */
    /**
     * 文件事件数组，存储已经注册的（需要监控的）文件事件
     */
    aeFileEvent *events; /* Registered events */
    /**
     * 存储被触发的文件事件
     */
    aeFiredEvent *fired; /* Fired events */
    /**
     * 时间事件链表的头节点
     *
     * redis有多个定时任务，所以会有多个时间事件，多个时间事件形成链表
     */
    aeTimeEvent *timeEventHead;
    /**
     * 标识事件循环是否结束
     */
    int stop;
    /**
     * redis底层可以使用4中IO多路复用模型，apidata是对四种模型的统一封装
     */
    void *apidata; /* This is used for polling API specific data */
    /**
     * redis服务器需要阻塞等待文件事件的发生，进程阻塞前会调用beforesleep函数
     */
    aeBeforeSleepProc *beforesleep;
    /**
     * redis服务器需要阻塞等待文件事件的发生，进程被唤醒后会调用aftersleep函数
     */
    aeBeforeSleepProc *aftersleep;
} aeEventLoop;

/* Prototypes */
aeEventLoop *aeCreateEventLoop(int setsize);
void aeDeleteEventLoop(aeEventLoop *eventLoop);
void aeStop(aeEventLoop *eventLoop);
int aeCreateFileEvent(aeEventLoop *eventLoop, int fd, int mask,
        aeFileProc *proc, void *clientData);
void aeDeleteFileEvent(aeEventLoop *eventLoop, int fd, int mask);
int aeGetFileEvents(aeEventLoop *eventLoop, int fd);
long long aeCreateTimeEvent(aeEventLoop *eventLoop, long long milliseconds,
        aeTimeProc *proc, void *clientData,
        aeEventFinalizerProc *finalizerProc);
int aeDeleteTimeEvent(aeEventLoop *eventLoop, long long id);
int aeProcessEvents(aeEventLoop *eventLoop, int flags);
int aeWait(int fd, int mask, long long milliseconds);
void aeMain(aeEventLoop *eventLoop);
char *aeGetApiName(void);
void aeSetBeforeSleepProc(aeEventLoop *eventLoop, aeBeforeSleepProc *beforesleep);
void aeSetAfterSleepProc(aeEventLoop *eventLoop, aeBeforeSleepProc *aftersleep);
int aeGetSetSize(aeEventLoop *eventLoop);
int aeResizeSetSize(aeEventLoop *eventLoop, int setsize);

#endif
