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

/* Basic data types. */
typedef int8_t      bque_s8_t;
typedef uint8_t     bque_u8_t;

typedef int16_t     bque_s16_t;
typedef uint16_t    bque_u16_t;

typedef int32_t     bque_s32_t;
typedef uint32_t    bque_u32_t;

typedef int64_t     bque_s64_t;
typedef uint64_t    bque_u64_t;

typedef uint32_t    bque_size_t;

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
} bque_iter_order_t;

/* sorting order. */
typedef enum _bque_sort_order {
    BQUE_SORT_ASCENDING     = 0,
    BQUE_SORT_DESCENDING    = 1,
} bque_sort_order_t;

/* sorting result. */
typedef enum _bque_sort_res {

    /* a is less than b. */
    BQUE_SORT_LESS      = -1,

    /* a is equal to b. */
    BQUE_SORT_EQUAL     = 0,

    /* a is greater than b. */
    BQUE_SORT_GREATER   = 1,
} bque_sort_res_t;

#ifdef BQUE_DEBUG

/* logging function for debug. */
#define BQUE_LOG(fmt, ...)  printf(fmt, ##__VA_ARGS__)

/* assertion macro used in the APIs. */
#define BQUE_ASSERT(expr)   \
    if (!(expr)) { BQUE_LOG("[BQUE] %s:%d: assertion failed: \"%s\"\n", \
        __FILE__, __LINE__, #expr); while (1);};

/* returned result used in the APIs. */
typedef enum _bque_res      bque_res_t;

#else

/* returned result used in the APIs. */
typedef bque_s32_t          bque_res_t;

/* assertion macro used in the APIs. */
#define BQUE_ASSERT(expr)

#endif

/* configuration of the buffer queue. */
typedef struct _bque_conf {
    bque_u32_t buff_num_max;
    bque_u32_t buff_size_max;
} bque_conf_t;

/* status of the buffer queue. */
typedef struct _bque_stat {
    bque_u32_t buff_num;
} bque_stat_t;

/* buffer information of the node. */
typedef struct _bque_buff {
    bque_u32_t size;
    bque_u8_t *ptr;
} bque_buff_t;

/* context of the buffer queue. */
typedef struct _bque_ctx    bque_ctx_t;

/* iterating callback. */
typedef bque_res_t (*bque_iter_cb_t)(bque_buff_t *buff, bque_u32_t idx, bque_u32_t num);

/* sorting callback. */
typedef bque_sort_res_t (*bque_sort_cb_t)(bque_buff_t *buff_a, bque_buff_t *buff_b);

bque_res_t bque_new(bque_ctx_t **ctx, bque_conf_t *conf);

bque_res_t bque_del(bque_ctx_t *ctx);

bque_res_t bque_stat(bque_ctx_t *ctx, bque_stat_t *stat);

bque_res_t bque_enqueue(bque_ctx_t *ctx, const void *buff, bque_u32_t size);

bque_res_t bque_preempt(bque_ctx_t *ctx, const void *buff, bque_u32_t size);

bque_res_t bque_insert(bque_ctx_t *ctx, bque_u32_t idx, const void *buff, bque_u32_t size);

bque_res_t bque_dequeue(bque_ctx_t *ctx, void *buff, bque_u32_t *size);

bque_res_t bque_forfeit(bque_ctx_t *ctx, void *buff, bque_u32_t *size);

bque_res_t bque_drop(bque_ctx_t *ctx, bque_u32_t idx, void *buff, bque_u32_t *size);

bque_res_t bque_empty(bque_ctx_t *ctx);

bque_res_t bque_item(bque_ctx_t *ctx, bque_s32_t idx, bque_buff_t *buff);

bque_res_t bque_sort(bque_ctx_t *ctx, bque_sort_cb_t cb, bque_sort_order_t order);

bque_res_t bque_foreach(bque_ctx_t *ctx, bque_iter_cb_t cb, bque_iter_order_t order);

#endif
