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
    bque_buff buff;
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
 * @param conf configuration pointer.
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

    /* empty the queue. */
    bque_empty(ctx);

    /* free context. */
    free(ctx);

    return BQUE_OK;
}

/**
 * @brief get the status of the queue.
 * 
 * @param ctx context pointer.
 * @param stat status pointer.
*/
bque_res bque_status(bque_ctx *ctx, bque_stat *stat) {
    bque_u32 node_num;

    BQUE_ASSERT(ctx != NULL);
    BQUE_ASSERT(stat != NULL);

    node_num = ctx->cache.node_num;
    stat->buff_num = node_num;
    if (ctx->head_node != NULL) {
        stat->head_buff_size = ctx->head_node->buff.size;
    }
    if (ctx->tail_node != NULL) {
        stat->tail_buff_size = ctx->tail_node->buff.size;
    }

    return BQUE_OK;
}

/**
 * @brief create a new node.
 * 
 * @param node the address of the node pointer.
 * @param size buffer size.
*/
static bque_res create_node(bque_node **node, bque_u32 size) {
    bque_node *alloc_node;
    bque_u8 *alloc_buff;

    BQUE_ASSERT(node != NULL);

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

    /* initialize node. */
    memset(alloc_node, 0, sizeof(bque_node));
    alloc_node->buff.ptr = alloc_buff;
    alloc_node->buff.size = size;

    /* return node. */
    *node = alloc_node;

    return BQUE_OK;
}

/**
 * @brief append a buffer to the tail of the queue.
 * 
 * @param ctx context pointer.
 * @param buff buffer pointer, can be NULL, which means the function will not
 *             copy the buffer.
 * @param size buffer size.
*/
bque_res bque_enqueue(bque_ctx *ctx, const void *buff, bque_u32 size) {
    bque_node *new_node;
    bque_res res;

    BQUE_ASSERT(ctx != NULL);

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

    /* create a new node. */
    res = create_node(&new_node, size);
    if (res != BQUE_OK) {
        return res;
    }

    /* if necessary, copy the buffer. */
    if (buff != NULL) {
        memcpy(new_node->buff.ptr, buff, size);
    }

    /* append the node. */
    if (ctx->tail_node == NULL) {
        ctx->head_node = new_node;
        ctx->tail_node = new_node;
        ctx->cache.node_num = 1;
    } else {
        new_node->prev_node = ctx->tail_node;
        ctx->tail_node->next_node = new_node;
        ctx->tail_node = new_node;
        ctx->cache.node_num++;
    }

    return BQUE_OK;
}

/**
 * @brief append a buffer to the head of the queue.
 * 
 * @param ctx context pointer.
 * @param buff buffer pointer, can be NULL, which means the function will not
 *             copy the buffer.
 * @param size buffer size.
*/
bque_res bque_preempt(bque_ctx *ctx, const void *buff, bque_u32 size) {
    bque_node *new_node;
    bque_res res;

    BQUE_ASSERT(ctx != NULL);

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

    /* create a new node. */
    res = create_node(&new_node, size);
    if (res != BQUE_OK) {
        return res;
    }

    /* if necessary, copy the buffer. */
    if (buff != NULL) {
        memcpy(new_node->buff.ptr, buff, size);
    }

    /* append the node. */
    if (ctx->head_node == NULL) {
        ctx->head_node = new_node;
        ctx->tail_node = new_node;
        ctx->cache.node_num = 1;
    } else {
        new_node->next_node = ctx->head_node;
        ctx->head_node->prev_node = new_node;
        ctx->head_node = new_node;
        ctx->cache.node_num++;
    }

    return BQUE_OK;
}

/**
 * @brief insert a buffer to the queue.
 * 
 * @note the buffer will be inserted to the specified index.
 * 
 * @param ctx context pointer.
 * @param idx index of the inserted buffer.
 * @param buff buffer pointer.
 * @param size buffer size.
*/
bque_res bque_insert(bque_ctx *ctx, bque_u32 idx, const void *buff, bque_u32 size) {
    bque_node *new_node;
    bque_res res;
    bque_u32 i;

    BQUE_ASSERT(ctx != NULL);

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

    /* check whether the idx is invalid. */
    if (idx > ctx->cache.node_num) {
        return BQUE_ERR_BAD_OFFS;
    }

    /* create a new node. */
    res = create_node(&new_node, size);
    if (res != BQUE_OK) {
        return res;
    }

    /* copy the buffer. */
    if (buff != NULL) {
        memcpy(new_node->buff.ptr, buff, size);
    }

    /* insert the node. */
    if (idx == 0) {

        /* insert to the head. */
        if (ctx->head_node == NULL) {
            ctx->head_node = new_node;
            ctx->tail_node = new_node;
        } else {
            new_node->next_node = ctx->head_node;
            ctx->head_node->prev_node = new_node;
            ctx->head_node = new_node;
        }
    } else if (idx == ctx->cache.node_num) {

        /* insert to the tail. */
        new_node->prev_node = ctx->tail_node;
        ctx->tail_node->next_node = new_node;
        ctx->tail_node = new_node;
    } else {

        /* insert to the middle. */
        bque_node *curt_node = ctx->head_node;

        for (i = 0; i < idx; i++) {
            curt_node = curt_node->next_node;
        }
        new_node->prev_node = curt_node->prev_node;
        new_node->next_node = curt_node;
        curt_node->prev_node->next_node = new_node;
        curt_node->prev_node = new_node;
    }

    /* update the node number. */
    ctx->cache.node_num++;

    return BQUE_OK;
}

/**
 * @brief detach a buffer from the head of the queue.
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
        memcpy(buff, ctx->head_node->buff.ptr, ctx->head_node->buff.size);
    }
    if (size != NULL) {
        *size = ctx->head_node->buff.size;
    }

    /* remove the node. */
    if (ctx->cache.node_num == 1) {
        free(ctx->head_node->buff.ptr);
        free(ctx->head_node);
        ctx->head_node = NULL;
        ctx->tail_node = NULL;
        ctx->cache.node_num = 0;
    } else {
        bque_node *curt_node;

        curt_node = ctx->head_node;
        ctx->head_node = curt_node->next_node;
        ctx->head_node->prev_node = NULL;
        free(curt_node->buff.ptr);
        free(curt_node);
        ctx->cache.node_num--;
    }

    return BQUE_OK;
}

/**
 * @brief detach a buffer from the tail of the queue.
 * 
 * @param ctx context pointer.
 * @param buff buffer pointer, when it's NULL, the buffer won't be copied.
 * @param size size pointer, when it's NULL, the size won't be copied.
*/
bque_res bque_forfeit(bque_ctx *ctx, void *buff, bque_u32 *size) {
    BQUE_ASSERT(ctx != NULL);

    /* check whether the queue is empty. */
    if (ctx->cache.node_num == 0) {
        return BQUE_ERR_EMPTY_QUE;
    }

    /* if necessary, output the buffer and buffer size of the head node. */
    if (buff != NULL) {
        memcpy(buff, ctx->tail_node->buff.ptr, ctx->tail_node->buff.size);
    }
    if (size != NULL) {
        *size = ctx->tail_node->buff.size;
    }

    /* remove the node. */
    if (ctx->cache.node_num == 1) {
        free(ctx->tail_node->buff.ptr);
        free(ctx->tail_node);
        ctx->head_node = NULL;
        ctx->tail_node = NULL;
        ctx->cache.node_num = 0;
    } else {
        bque_node *curt_node;

        curt_node = ctx->tail_node;
        ctx->tail_node = curt_node->prev_node;
        ctx->tail_node->next_node = NULL;
        free(curt_node->buff.ptr);
        free(curt_node);
        ctx->cache.node_num--;
    }

    return BQUE_OK;
}

/**
 * @brief detach a buffer from the queue by index.
 * 
 * @param ctx context pointer.
 * @param idx index of the buffer to be detached.
 * @param buff buffer pointer, when it's NULL, the buffer won't be copied.
 * @param size size pointer, when it's NULL, the size won't be copied.
*/
bque_res bque_drop(bque_ctx *ctx, bque_u32 idx, void *buff, bque_u32 *size) {
    bque_node *curt_node;
    bque_u32 curt_idx;

    BQUE_ASSERT(ctx != NULL);

    /* check whether the queue is empty. */
    if (ctx->cache.node_num == 0) {
        return BQUE_ERR_EMPTY_QUE;
    }

    /* check whether the index is valid. */
    if (idx >= ctx->cache.node_num) {
        return BQUE_ERR_BAD_IDX;
    }

    /* find the node. */
    curt_node = ctx->head_node;
    curt_idx = 0;
    while (curt_idx < idx) {
        curt_node = curt_node->next_node;
        curt_idx++;
    }

    /* if necessary, output the buffer and buffer size of the node. */
    if (buff != NULL) {
        memcpy(buff, curt_node->buff.ptr, curt_node->buff.size);
    }
    if (size != NULL) {
        *size = curt_node->buff.size;
    }

    /* remove the node. */
    if (ctx->cache.node_num == 1) {
        free(curt_node->buff.ptr);
        free(curt_node);
        ctx->head_node = NULL;
        ctx->tail_node = NULL;
        ctx->cache.node_num = 0;
    } else {
        if (curt_node == ctx->head_node) {
            ctx->head_node = curt_node->next_node;
            ctx->head_node->prev_node = NULL;
        } else if (curt_node == ctx->tail_node) {
            ctx->tail_node = curt_node->prev_node;
            ctx->tail_node->next_node = NULL;
        } else {
            curt_node->prev_node->next_node = curt_node->next_node;
            curt_node->next_node->prev_node = curt_node->prev_node;
        }
        free(curt_node->buff.ptr);
        free(curt_node);
        ctx->cache.node_num--;
    }

    return BQUE_OK;
}

/**
 * @brief empty the queue.
 * 
 * @param ctx context pointer.
*/
bque_res bque_empty(bque_ctx *ctx) {
    bque_node *curt_node;
    bque_node *next_node;

    BQUE_ASSERT(ctx != NULL);

    /* check whether the queue is empty. */
    if (ctx->cache.node_num == 0) {
        return BQUE_OK;
    }

    /* remove all nodes. */
    curt_node = ctx->head_node;
    while (curt_node != NULL) {
        next_node = curt_node->next_node;
        free(curt_node->buff.ptr);
        free(curt_node);
        curt_node = next_node;
    }

    /* reset context. */
    ctx->head_node = NULL;
    ctx->tail_node = NULL;
    ctx->cache.node_num = 0;

    return BQUE_OK;
}

/**
 * @brief get buffer in the queue by index.
 * 
 * @note note that you should create a bque_buff variable and then
 *       pass its address as the argument to call this function,
 *       after this function is returned, you can use buff.size and
 *       buff.ptr to access the indexed buffer size and pointer.
 * 
 * @param ctx context pointer.
 * @param idx buffer index, 0 and positive value means forward index, negative
 *            value means backward index.
 * @param buff pointer pointing to the buffer information structure.
*/
bque_res bque_item(bque_ctx *ctx, bque_s32 idx, bque_buff *buff) {
    bque_u32 node_num;
    bque_u32 curt_node_idx;
    bque_u32 forward_node_idx;
    bque_node *curt_node;

    BQUE_ASSERT(ctx != NULL);
    BQUE_ASSERT(buff != NULL);

    /* check whether the index is valid. */
    node_num = ctx->cache.node_num;
    if (idx >= 0) {
        forward_node_idx = (bque_u32)idx;
        if (forward_node_idx > node_num - 1) {
            return BQUE_ERR_BAD_IDX;
        }
    } else {
        bque_u32 tmp;

        tmp = (bque_u32)(-idx);
        if (tmp > node_num) {
            return BQUE_ERR_BAD_IDX;
        }

        forward_node_idx = node_num - tmp;
    }

    /* find the indexed node. */
    curt_node_idx = 0;
    curt_node = ctx->head_node;
    while (curt_node_idx != forward_node_idx) {
        curt_node = curt_node->next_node;
        curt_node_idx++;
    }

    /* copy the buffer information. */
    memcpy(buff, &curt_node->buff, sizeof(bque_buff));

    return BQUE_OK;
}

/**
 * @brief sort the buffer queue.
 * 
 * @note this function will sort the queue in specified order, and the callback
 *       function will be called to compare two buffers, the callback function
 *       should return BQUE_SORT_EQUAL if the two buffers are equal, and return
 *       BQUE_SORT_GREATER if the first buffer is greater than the second one,
 *       and return BQUE_SORT_LESS if the first buffer is less than the second
 *       one.
 * 
 * @param ctx context pointer.
 * @param cb sorting callback, used to compare two buffers.
 * @param order sorting order, BQUE_SORT_ASCENDING or BQUE_SORT_DESCENDING.
*/
bque_res bque_sort(bque_ctx *ctx, bque_sort_cb cb, bque_sort_order order) {
    bque_sort_res sort_res;
    bque_node **node_array;
    bque_node *curt_node;
    bque_node *temp_node;
    bque_u32 node_num;
    bque_u32 node_idx;
    bque_buff buff_a;
    bque_buff buff_b;

    BQUE_ASSERT(ctx != NULL);
    BQUE_ASSERT(cb != NULL);
    BQUE_ASSERT(order == BQUE_SORT_ASCENDING ||
                order == BQUE_SORT_DESCENDING);

    /* check whether the queue is empty. */
    node_num = ctx->cache.node_num;
    if (node_num == 0) {
        return BQUE_ERR_EMPTY_QUE;
    } else if (node_num == 1) {
        return BQUE_OK;
    }

    /* allocate memory for the node array. */
    node_array = (bque_node **)malloc(sizeof(bque_node *) * node_num);
    if (node_array == NULL) {
        return BQUE_ERR_NO_MEM;
    }

    /* copy all the pointers of all nodes to the array. */
    curt_node = ctx->head_node;
    node_idx = 0;
    do {
        node_array[node_idx] = curt_node;
        curt_node = curt_node->next_node;
        node_idx++;
    } while (curt_node != NULL);

    /* sort the node array by the specified order using bubble sort. */
    if (order == BQUE_SORT_ASCENDING) {
        for (bque_u32 i = 0; i < node_num - 1; i++) {
            for (bque_u32 j = 0; j < node_num - i - 1; j++) {
                memcpy(&buff_a, &node_array[j]->buff, sizeof(bque_buff));
                memcpy(&buff_b, &node_array[j + 1]->buff, sizeof(bque_buff));
                sort_res = cb(&buff_a, &buff_b);
                if (sort_res == BQUE_SORT_GREATER) {
                    temp_node = node_array[j];
                    node_array[j] = node_array[j + 1];
                    node_array[j + 1] = temp_node;
                }
            }
        }
    } else {
        for (bque_u32 i = 0; i < node_num - 1; i++) {
            for (bque_u32 j = 0; j < node_num - i - 1; j++) {
                memcpy(&buff_a, &node_array[j]->buff, sizeof(bque_buff));
                memcpy(&buff_b, &node_array[j + 1]->buff, sizeof(bque_buff));
                sort_res = cb(&buff_a, &buff_b);
                if (sort_res == BQUE_SORT_LESS) {
                    temp_node = node_array[j];
                    node_array[j] = node_array[j + 1];
                    node_array[j + 1] = temp_node;
                }
            }
        }
    }

    /* link all the nodes in the array. */
    for (bque_u32 i = 0; i < node_num; i++) {
        if (i == 0) {
            ctx->head_node = node_array[0];
            node_array[0]->prev_node = NULL;
            node_array[0]->next_node = node_array[1];
        } else if (i == node_num - 1) {
            ctx->tail_node = node_array[i];
            node_array[i]->prev_node = node_array[i - 1];
            node_array[i]->next_node = NULL;
        } else {
            node_array[i]->prev_node = node_array[i - 1];
            node_array[i]->next_node = node_array[i + 1];
        }
    }

    /* free the node array. */
    free(node_array);

    return BQUE_OK;
}

/**
 * @brief iterate through the queue in specified order.
 * 
 * @note this function will call the callback function for each buffer, note
 *       that the callback function should return BQUE_OK to continue iterating,
 *       then after all buffers are iterated, this function will return BQUE_OK,
 *       or the callback function may return BQUE_ERR_ITER_STOP to stop
 *       iterating, then this function will return BQUE_ERR_ITER_STOP
 *       immediately.
 * 
 * @param ctx context pointer.
 * @param cb iterating callback.
 * @param order iterating order.
*/
bque_res bque_foreach(bque_ctx *ctx, bque_iter_cb cb, bque_iter_order order) {
    bque_u32 node_num;
    bque_u32 node_idx;
    bque_node *curt_node;
    bque_buff node_buff;
    bque_res res;

    BQUE_ASSERT(ctx != NULL);
    BQUE_ASSERT(cb != NULL);
    BQUE_ASSERT(order == BQUE_ITER_FORWARD ||
                order == BQUE_ITER_BACKWARD);

    /* if there is no any node in this queue, return immediately. */
    node_num = ctx->cache.node_num;
    if (node_num == 0) {
        return BQUE_OK;
    }

    /* iterate through the queue in specified order. */
    if (order == BQUE_ITER_FORWARD) {
        curt_node = ctx->head_node;
        node_idx = 0;
        do {
            memcpy(&node_buff, &curt_node->buff, sizeof(bque_buff));
            res = cb(&node_buff, node_idx, node_num);
            if (res == BQUE_ERR_ITER_STOP) {
                return BQUE_ERR_ITER_STOP;
            }

            curt_node = curt_node->next_node;
            node_idx++;
        } while (curt_node != NULL);
    } else {
        curt_node = ctx->tail_node;
        node_idx = node_num - 1;
        do {
            memcpy(&node_buff, &curt_node->buff, sizeof(bque_buff));
            res = cb(&node_buff, node_idx, node_num);
            if (res == BQUE_ERR_ITER_STOP) {
                return BQUE_ERR_ITER_STOP;
            }

            curt_node = curt_node->prev_node;
            node_idx--;
        } while (curt_node != NULL);
    }

    return BQUE_OK;
}
