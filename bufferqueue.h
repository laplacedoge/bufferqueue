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

#include <stdint.h>

#ifdef BQUE_DEBUG

#include <stdarg.h>
#include <stdio.h>

#endif

/* basic data types. */
typedef int8_t      bque_s8;
typedef uint8_t     bque_u8;

typedef int16_t     bque_s16;
typedef uint16_t    bque_u16;

typedef int32_t     bque_s32;
typedef uint32_t    bque_u32;

typedef int64_t     bque_s64;
typedef uint64_t    bque_u64;

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

    /* invalid index. */
    BQUE_ERR_BAD_IDX    = -7,

    /* iterating stoped. */
    BQUE_ERR_ITER_STOP  = -8,
};

/* iterating order. */
typedef enum _bque_iter_order {
    BQUE_ITER_FORWARD   = 0,
    BQUE_ITER_BACKWARD  = 1,
} bque_iter_order;

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

/* buffer information of the node. */
typedef struct _bque_buff {
    bque_u32 size;
    bque_u8 *ptr;
} bque_buff;

/* context of the buffer queue. */
typedef struct _bque_ctx    bque_ctx;

/* iterating callback. */
typedef bque_res (*bque_iter_cb)(bque_buff *buff, bque_u32 idx, bque_u32 num);

bque_res bque_new(bque_ctx **ctx, bque_conf *conf);

bque_res bque_del(bque_ctx *ctx);

bque_res bque_status(bque_ctx *ctx, bque_stat *stat);

bque_res bque_enqueue(bque_ctx *ctx, const void *buff, bque_u32 size);

bque_res bque_preempt(bque_ctx *ctx, const void *buff, bque_u32 size);

bque_res bque_dequeue(bque_ctx *ctx, void *buff, bque_u32 *size);

bque_res bque_forfeit(bque_ctx *ctx, void *buff, bque_u32 *size);

bque_res bque_peek(bque_ctx *ctx, void *buff, bque_u32 offs, bque_u32 size);

bque_res bque_item(bque_ctx *ctx, bque_s32 idx, bque_buff *buff);

bque_res bque_foreach(bque_ctx *ctx, bque_iter_cb cb, bque_iter_order order);

#endif
