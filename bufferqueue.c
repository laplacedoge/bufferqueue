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

#include "bufferqueue.h"

#include <stdlib.h>
#include <string.h>

/* node of the buffer queue. */
typedef struct _bque_node   bque_node;

struct _bque_node {
    bque_node *prev_node;
    bque_node *next_node;
    bque_u32 buff_size;
    bque_u8 *buff;
};

/* context of the buffer queue. */
struct _bque_ctx {
    bque_node *head_node;
    bque_node *tail_node;
    struct _bque_ctx_conf {
        bque_u32 node_num_max;
        bque_u32 buff_size_max;
    } conf;
    struct _bque_ctx_cache {
        bque_u32 node_num;
    } cache;
};

/* default maximum number of the node in a queue. */
#define BQUE_DEF_NODE_NUM_MAX       1024

/* default maximum number of the node in a queue. */
#define BQUE_DEF_BUFF_SIZE_MAX      1024

/**
 * @brief create a queue.
 * 
 * @param ctx the address of the context pointer.
*/
bque_res bque_new(bque_ctx **ctx, bque_conf *conf) {
    bque_ctx *alloc_ctx;

    BQUE_ASSERT(ctx != NULL);

    /* allocate context. */
    alloc_ctx = (bque_ctx *)malloc(sizeof(bque_ctx));
    if (alloc_ctx == NULL)
    {
        return BQUE_ERR_NO_MEM;
    }

    /* initialize context. */
    memset(alloc_ctx, 0, sizeof(bque_ctx));

    /* configure context. */
    if (conf != NULL) {
        alloc_ctx->conf.node_num_max = conf->buff_num_max;
        alloc_ctx->conf.buff_size_max = conf->buff_size_max;
    } else {
        alloc_ctx->conf.node_num_max = BQUE_DEF_NODE_NUM_MAX;
        alloc_ctx->conf.buff_size_max = BQUE_DEF_BUFF_SIZE_MAX;
    }

    *ctx = alloc_ctx;

    return BQUE_OK;
}

/**
 * @brief delete the queue.
 * 
 * @param ctx context pointer.
*/
bque_res bque_del(bque_ctx *ctx) {
    BQUE_ASSERT(ctx != NULL);

    /* free all the node and its buffer. */
    if (ctx->cache.node_num > 0) {
        bque_node *curt_node;
        bque_node *next_node;

        curt_node = ctx->head_node;
        while (curt_node != NULL) {
            next_node = curt_node->next_node;
            free(curt_node->buff);
            free(curt_node);
            curt_node = next_node;
        }
    }

    /* free context. */
    free(ctx);

    return BQUE_OK;
}

/**
 * @brief get the status of the queue.
 * 
 * @param ctx context pointer.
 * @param size status pointer.
*/
bque_res bque_status(bque_ctx *ctx, bque_stat *stat) {
    bque_u32 node_num;

    BQUE_ASSERT(ctx != NULL);
    BQUE_ASSERT(stat != NULL);

    node_num = ctx->cache.node_num;
    stat->buff_num = node_num;
    if (ctx->head_node != NULL) {
        stat->head_buff_size = ctx->head_node->buff_size;
    }
    if (ctx->tail_node != NULL) {
        stat->tail_buff_size = ctx->tail_node->buff_size;
    }

    return BQUE_OK;
}

/**
 * @brief enqueue a buffer to the tail of the queue.
 * 
 * @param ctx context pointer.
 * @param buff buffer pointer.
 * @param size buffer size.
*/
bque_res bque_enqueue(bque_ctx *ctx, const void *buff, bque_u32 size) {
    bque_node *alloc_node;
    bque_u8 *alloc_buff;

    BQUE_ASSERT(ctx != NULL);
    BQUE_ASSERT(buff != NULL);

    /* check whether the queue is full. */
    if (ctx->conf.node_num_max != 0 &&
        ctx->cache.node_num + 1 > ctx->conf.node_num_max) {
        return BQUE_ERR_FULL_QUE;
    }

    /* check whether the size is invalid. */
    if (size == 0 || (ctx->conf.buff_size_max != 0 &&
                      size > ctx->conf.buff_size_max)) {
        return BQUE_ERR_BAD_SIZE;
    }

    /* allocate buffer. */
    alloc_buff = (bque_u8 *)malloc(size);
    if (alloc_buff == NULL) {
        return BQUE_ERR_NO_MEM;
    }

    /* allocate node. */
    alloc_node = (bque_node *)malloc(sizeof(bque_node));
    if (alloc_node == NULL) {
        free(alloc_buff);

        return BQUE_ERR_NO_MEM;
    }

    /* initialize node and copy buffer. */
    memset(alloc_node, 0, sizeof(bque_node));
    memcpy(alloc_buff, buff, size);
    alloc_node->buff = alloc_buff;
    alloc_node->buff_size = size;

    /* enqueue the node. */
    if (ctx->head_node == NULL) {
        ctx->head_node = alloc_node;
        ctx->tail_node = alloc_node;
        ctx->cache.node_num = 1;
    } else {
        alloc_node->prev_node = ctx->tail_node;
        ctx->tail_node->next_node = alloc_node;
        ctx->tail_node = alloc_node;
        ctx->cache.node_num++;
    }

    return BQUE_OK;
}

/**
 * @brief dequeue the head buffer from the queue.
 * 
 * @param ctx context pointer.
 * @param buff buffer pointer, when it's NULL, the buffer won't be copied.
 * @param size size pointer, when it's NULL, the size won't be copied.
*/
bque_res bque_dequeue(bque_ctx *ctx, void *buff, bque_u32 *size) {
    BQUE_ASSERT(ctx != NULL);

    /* check whether the queue is empty. */
    if (ctx->cache.node_num == 0) {
        return BQUE_ERR_EMPTY_QUE;
    }

    /* if necessary, output the buffer and buffer size of the head node. */
    if (buff != NULL) {
        memcpy(buff, ctx->head_node->buff, ctx->head_node->buff_size);
    }
    if (size != NULL) {
        *size = ctx->head_node->buff_size;
    }

    /* remove the node from the queue. */
    if (ctx->cache.node_num == 1) {
        free(ctx->head_node->buff);
        free(ctx->head_node);
        ctx->head_node = NULL;
        ctx->tail_node = NULL;
        ctx->cache.node_num = 0;
    } else {
        bque_node *curt_node;

        curt_node = ctx->head_node;
        ctx->head_node = curt_node->next_node;
        ctx->head_node->prev_node = NULL;
        free(curt_node->buff);
        free(curt_node);
        ctx->cache.node_num--;
    }

    return BQUE_OK;
}

/**
 * @brief peek part of the data of the head buffer from the queue.
 * 
 * @note this operation won't dequeue the head buffer from the queue.
 * 
 * @param ctx context pointer.
 * @param buff buffer pointer, when it's NULL, the buffer won't be copied.
 * @param size size pointer, when it's NULL, the size won't be copied.
*/
bque_res bque_peek(bque_ctx *ctx, void *buff, bque_u32 offs, bque_u32 size) {
    BQUE_ASSERT(ctx != NULL);
    BQUE_ASSERT(buff != NULL);

    /* check whether the queue is empty. */
    if (ctx->cache.node_num == 0) {
        return BQUE_ERR_EMPTY_QUE;
    }

    /* check whether the offset is valid. */
    if (offs > ctx->head_node->buff_size) {
        return BQUE_ERR_BAD_OFFS;
    }

    /* check whether the size is valid. */
    if (offs + size > ctx->head_node->buff_size) {
        return BQUE_ERR_BAD_SIZE;
    }

    /* copy buffer. */
    memcpy(buff, ctx->head_node->buff + offs, size);

    return BQUE_OK;
}
