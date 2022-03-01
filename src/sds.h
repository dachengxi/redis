/* SDSLib 2.0 -- A C dynamic strings library
 *
 * Copyright (c) 2006-2015, Salvatore Sanfilippo <antirez at gmail dot com>
 * Copyright (c) 2015, Oran Agra
 * Copyright (c) 2015, Redis Labs, Inc
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

#ifndef __SDS_H
#define __SDS_H

#define SDS_MAX_PREALLOC (1024*1024)
extern const char *SDS_NOINIT;

#include <sys/types.h>
#include <stdarg.h>
#include <stdint.h>

/*
 * redis自定义的简单动态字符串，除了用来保存字符串值，还可以用作缓冲区：
 * AOF模块中的AOF缓冲区以及客户端状态中的输入缓冲区。
 */
typedef char *sds;

/*
 * sdshdr5用来存储长度小于32的短字符串
 * flags占1个字节：
 *      - 低3位表示存储类型，
 *      - 高5位表示存储长度，2^5 - 1
 */
/* Note: sdshdr5 is never used, we just access the flags byte directly.
 * However is here to document the layout of type 5 SDS strings. */
struct __attribute__ ((__packed__)) sdshdr5 {
    // flags占1个字节：低3位存储类型，高5位存储长度
    unsigned char flags; /* 3 lsb of type, and 5 msb of string length */
    // 实际存储字符串的字节数组
    char buf[];
};
struct __attribute__ ((__packed__)) sdshdr8 {
    // 字符串的实际长度，不包含最后的空字符，1个字节
    uint8_t len; /* used */
    // 字符串最大长度，也就是buf分配的总长度，不包含header大小和最后的空字符，1个字节
    uint8_t alloc; /* excluding the header and null terminator */
    // header的类型标志，低3位表示存储类型，高5位未使用，1个字节
    unsigned char flags; /* 3 lsb of type, 5 unused bits */
    // 实际存储字符串的字节数组
    char buf[];
};
struct __attribute__ ((__packed__)) sdshdr16 {
    // 字符串的实际长度，不包含最后的空字符，2个字节
    uint16_t len; /* used */
    // 字符串最大长度，也就是buf分配的总长度，不包含header大小和最后的空字符，2个字节
    uint16_t alloc; /* excluding the header and null terminator */
    // header的类型标志，低3位表示存储类型，高5位未使用，1个字节
    unsigned char flags; /* 3 lsb of type, 5 unused bits */
    // 实际存储字符串的字节数组
    char buf[];
};
struct __attribute__ ((__packed__)) sdshdr32 {
    // 字符串的实际长度，不包含最后的空字符，4个字节
    uint32_t len; /* used */
    // 字符串最大长度，也就是buf分配的总长度，不包含header大小和最后的空字符，4个字节
    uint32_t alloc; /* excluding the header and null terminator */
    // header的类型标志，低3位表示存储类型，高5位未使用，1个字节
    unsigned char flags; /* 3 lsb of type, 5 unused bits */
    // 实际存储字符串的字节数组
    char buf[];
};
struct __attribute__ ((__packed__)) sdshdr64 {
    // 字符串的实际长度，不包含最后的空字符，8个字节
    uint64_t len; /* used */
    // 字符串最大长度，也就是buf分配的总长度，不包含header大小和最后的空字符，8个字节
    uint64_t alloc; /* excluding the header and null terminator */
    // header的类型标志，低3位表示存储类型，高5位未使用，1个字节
    unsigned char flags; /* 3 lsb of type, 5 unused bits */
    // 实际存储字符串的字节数组
    char buf[];
};

#define SDS_TYPE_5  0
#define SDS_TYPE_8  1
#define SDS_TYPE_16 2
#define SDS_TYPE_32 3
#define SDS_TYPE_64 4
#define SDS_TYPE_MASK 7
#define SDS_TYPE_BITS 3
// 得到执行sdshdr的起始地址的指针
#define SDS_HDR_VAR(T,s) struct sdshdr##T *sh = (void*)((s)-(sizeof(struct sdshdr##T)));
// 得到sdshdr的起始地址
#define SDS_HDR(T,s) ((struct sdshdr##T *)((s)-(sizeof(struct sdshdr##T))))
#define SDS_TYPE_5_LEN(f) ((f)>>SDS_TYPE_BITS)

/**
 * 获取sds字符串长度
 * @param s
 * @return
 */
static inline size_t sdslen(const sds s) {
    unsigned char flags = s[-1];
    switch(flags&SDS_TYPE_MASK) {
        case SDS_TYPE_5:
            return SDS_TYPE_5_LEN(flags);
        case SDS_TYPE_8:
            return SDS_HDR(8,s)->len;
        case SDS_TYPE_16:
            return SDS_HDR(16,s)->len;
        case SDS_TYPE_32:
            return SDS_HDR(32,s)->len;
        case SDS_TYPE_64:
            return SDS_HDR(64,s)->len;
    }
    return 0;
}

/**
 * 返回sds的buf中可用的长度，alloc减去len得到的长度
 * @param s
 * @return
 */
static inline size_t sdsavail(const sds s) {
    unsigned char flags = s[-1];
    switch(flags&SDS_TYPE_MASK) {
        case SDS_TYPE_5: {
            return 0;
        }
        case SDS_TYPE_8: {
            SDS_HDR_VAR(8,s);
            return sh->alloc - sh->len;
        }
        case SDS_TYPE_16: {
            SDS_HDR_VAR(16,s);
            return sh->alloc - sh->len;
        }
        case SDS_TYPE_32: {
            SDS_HDR_VAR(32,s);
            return sh->alloc - sh->len;
        }
        case SDS_TYPE_64: {
            SDS_HDR_VAR(64,s);
            return sh->alloc - sh->len;
        }
    }
    return 0;
}

/**
 * 设置sds的长度
 * @param s
 * @param newlen
 */
static inline void sdssetlen(sds s, size_t newlen) {
    unsigned char flags = s[-1];
    switch(flags&SDS_TYPE_MASK) {
        case SDS_TYPE_5:
            {
                unsigned char *fp = ((unsigned char*)s)-1;
                *fp = SDS_TYPE_5 | (newlen << SDS_TYPE_BITS);
            }
            break;
        case SDS_TYPE_8:
            SDS_HDR(8,s)->len = newlen;
            break;
        case SDS_TYPE_16:
            SDS_HDR(16,s)->len = newlen;
            break;
        case SDS_TYPE_32:
            SDS_HDR(32,s)->len = newlen;
            break;
        case SDS_TYPE_64:
            SDS_HDR(64,s)->len = newlen;
            break;
    }
}

static inline void sdsinclen(sds s, size_t inc) {
    unsigned char flags = s[-1];
    switch(flags&SDS_TYPE_MASK) {
        case SDS_TYPE_5:
            {
                unsigned char *fp = ((unsigned char*)s)-1;
                unsigned char newlen = SDS_TYPE_5_LEN(flags)+inc;
                *fp = SDS_TYPE_5 | (newlen << SDS_TYPE_BITS);
            }
            break;
        case SDS_TYPE_8:
            SDS_HDR(8,s)->len += inc;
            break;
        case SDS_TYPE_16:
            SDS_HDR(16,s)->len += inc;
            break;
        case SDS_TYPE_32:
            SDS_HDR(32,s)->len += inc;
            break;
        case SDS_TYPE_64:
            SDS_HDR(64,s)->len += inc;
            break;
    }
}

/* sdsalloc() = sdsavail() + sdslen() */
static inline size_t sdsalloc(const sds s) {
    unsigned char flags = s[-1];
    switch(flags&SDS_TYPE_MASK) {
        case SDS_TYPE_5:
            return SDS_TYPE_5_LEN(flags);
        case SDS_TYPE_8:
            return SDS_HDR(8,s)->alloc;
        case SDS_TYPE_16:
            return SDS_HDR(16,s)->alloc;
        case SDS_TYPE_32:
            return SDS_HDR(32,s)->alloc;
        case SDS_TYPE_64:
            return SDS_HDR(64,s)->alloc;
    }
    return 0;
}

static inline void sdssetalloc(sds s, size_t newlen) {
    unsigned char flags = s[-1];
    switch(flags&SDS_TYPE_MASK) {
        case SDS_TYPE_5:
            /* Nothing to do, this type has no total allocation info. */
            break;
        case SDS_TYPE_8:
            SDS_HDR(8,s)->alloc = newlen;
            break;
        case SDS_TYPE_16:
            SDS_HDR(16,s)->alloc = newlen;
            break;
        case SDS_TYPE_32:
            SDS_HDR(32,s)->alloc = newlen;
            break;
        case SDS_TYPE_64:
            SDS_HDR(64,s)->alloc = newlen;
            break;
    }
}

/**
 * 创建sds
 * @param init
 * @param initlen
 * @return
 */
sds sdsnewlen(const void *init, size_t initlen);
/**
 * 根据给定的字符串创建sds
 * @param init
 * @return
 */
sds sdsnew(const char *init);
/**
 * 创建一个空字符串，长度为0，内容为:""
 * @return
 */
sds sdsempty(void);
/**
 * 复制给定的sds字符串
 * @param s
 * @return
 */
sds sdsdup(const sds s);
/**
 * 释放sds的内存
 * @param s
 */
void sdsfree(sds s);
/**
 * 将sds扩容到指定长度，并用0填充
 * @param s
 * @param len
 * @return
 */
sds sdsgrowzero(sds s, size_t len);
sds sdscatlen(sds s, const void *t, size_t len);
sds sdscat(sds s, const char *t);
/**
 * 拼接字符串
 * @param s
 * @param t
 * @return
 */
sds sdscatsds(sds s, const sds t);
/**
 * 将给定的c字符串复制到sds中
 * @param s
 * @param t
 * @param len
 * @return
 */
sds sdscpylen(sds s, const char *t, size_t len);
/**
 * 将给定的c字符串复制到sds中
 * @param s
 * @param t
 * @return
 */
sds sdscpy(sds s, const char *t);

sds sdscatvprintf(sds s, const char *fmt, va_list ap);
#ifdef __GNUC__
sds sdscatprintf(sds s, const char *fmt, ...)
    __attribute__((format(printf, 2, 3)));
#else
sds sdscatprintf(sds s, const char *fmt, ...);
#endif

sds sdscatfmt(sds s, char const *fmt, ...);
/**
 * 从sds两端清除所有给定的字符
 * @param s
 * @param cset
 * @return
 */
sds sdstrim(sds s, const char *cset);
void sdsrange(sds s, ssize_t start, ssize_t end);
void sdsupdatelen(sds s);
/**
 * 不直接释放内存，重置len值
 * @param s
 */
void sdsclear(sds s);
/**
 * 比较两个给定的sds的实际大小
 * @param s1
 * @param s2
 * @return
 */
int sdscmp(const sds s1, const sds s2);
/**
 * 按照给定的分隔符对sds进行切分
 * @param s
 * @param len
 * @param sep
 * @param seplen
 * @param count
 * @return
 */
sds *sdssplitlen(const char *s, ssize_t len, const char *sep, int seplen, int *count);
void sdsfreesplitres(sds *tokens, int count);
void sdstolower(sds s);
void sdstoupper(sds s);
sds sdsfromlonglong(long long value);
sds sdscatrepr(sds s, const char *p, size_t len);
sds *sdssplitargs(const char *line, int *argc);
sds sdsmapchars(sds s, const char *from, const char *to, size_t setlen);
sds sdsjoin(char **argv, int argc, char *sep);
sds sdsjoinsds(sds *argv, int argc, const char *sep, size_t seplen);

/* Low level functions exposed to the user API */
/**
 * 检查sds是否需要扩容，如果需要则进行扩容并返回新的sds
 * @param s
 * @param addlen
 * @return
 */
sds sdsMakeRoomFor(sds s, size_t addlen);
void sdsIncrLen(sds s, ssize_t incr);
/**
 * sds缩容
 * @param s
 * @return
 */
sds sdsRemoveFreeSpace(sds s);
/**
 * 返回给定的SDS当前占用的内存大小
 * @param s
 * @return
 */
size_t sdsAllocSize(sds s);
void *sdsAllocPtr(sds s);

/* Export the allocator used by SDS to the program using SDS.
 * Sometimes the program SDS is linked to, may use a different set of
 * allocators, but may want to allocate or free things that SDS will
 * respectively free or allocate. */
void *sds_malloc(size_t size);
void *sds_realloc(void *ptr, size_t size);
/**
 * sds释放内存
 * @param ptr
 */
void sds_free(void *ptr);

#ifdef REDIS_TEST
int sdsTest(int argc, char *argv[]);
#endif

#endif
