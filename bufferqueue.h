/**
 * MIT License
 * 
 * Copyright (c) 2023 Alex Chen
 * 
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 * 
 * The above copyright notice and this permission notice shall be included in
 * all copies or substantial portions of the Software.
 * 
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
 * SOFTWARE.
 */

#ifndef __BQUE_H__
#define __BQUE_H__

#ifdef BQUE_DEBUG

#include <stdarg.h>
#include <stdio.h>

#endif

/* basic data types. */
typedef signed char     bque_s8;
typedef unsigned char   bque_u8;

typedef signed short    bque_s16;
typedef unsigned short  bque_u16;

typedef signed int      bque_s32;
typedef unsigned int    bque_u32;

enum _bque_res {

    /* all is well, so far :) */
    BQUE_OK             = 0,

    /* generic error occurred. */
    BQUE_ERR            = -1,

    /* failed to allocate memory. */
    BQUE_ERR_NO_MEM     = -2,

    /* invalid offset. */
    BQUE_ERR_BAD_OFFS   = -3,

    /* invalid size. */
    BQUE_ERR_BAD_SIZE   = -4,

    /* full queue. */
    BQUE_ERR_FULL_QUE   = -5,

    /* empty queue. */
    BQUE_ERR_EMPTY_QUE  = -6,
};

#ifdef BQUE_DEBUG

/* logging function for debug. */
#define BQUE_LOG(fmt, ...)  printf(fmt, ##__VA_ARGS__)

/* assertion macro used in the APIs. */
#define BQUE_ASSERT(expr)   \
    if (!(expr)) { BQUE_LOG("[BQUE] %s:%d: assertion failed: \"%s\"\n", \
        __FILE__, __LINE__, #expr); while (1);};

/* returned result used in the APIs. */
typedef enum _bque_res      bque_res;

#else

/* returned result used in the APIs. */
typedef bque_s32            bque_res;

/* assertion macro used in the APIs. */
#define BQUE_ASSERT(expr)

#endif

/* configuration of the buffer queue. */
typedef struct _bque_conf {
    bque_u32 buff_num_max;
    bque_u32 buff_size_max;
} bque_conf;

typedef struct _bque_stat {
    bque_u32 buff_num;
    bque_u32 head_buff_size;
    bque_u32 tail_buff_size;
} bque_stat;

/* context of the buffer queue. */
typedef struct _bque_ctx    bque_ctx;

bque_res bque_new(bque_ctx **ctx, bque_conf *conf);

bque_res bque_del(bque_ctx *ctx);

bque_res bque_status(bque_ctx *ctx, bque_stat *stat);

bque_res bque_enqueue(bque_ctx *ctx, const void *buff, bque_u32 size);

bque_res bque_dequeue(bque_ctx *ctx, void *buff, bque_u32 *size);

bque_res bque_peek(bque_ctx *ctx, void *buff, bque_u32 offs, bque_u32 size);

#endif
