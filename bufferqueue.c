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
typedef struct _bque_node   bque_node_t;

struct _bque_node {
    bque_node_t *prev_node;
    bque_node_t *next_node;
    bque_u8_t *buff;
    bque_size_t size;
};

/* context of the buffer queue. */
struct _bque_ctx {
    bque_node_t *head_node;
    bque_node_t *tail_node;
    struct _bque_ctx_conf {
        bque_u32_t node_num_max;
        bque_u32_t buff_size_max;
        bque_free_buff_cb_t free_buff_cb;
    } conf;
    struct _bque_ctx_cache {
        bque_u32_t node_num;

        /* fast indexing cache to access the last indexed node. */
        struct _bque_ctx_cache_last {
            bque_node_t *node;
            bque_u32_t node_idx;
        } last;
    } cache;
};

/* default maximum number of the node in a queue. */
#define BQUE_DEF_NODE_NUM_MAX       1024

/* default maximum number of the node in a queue. */
#define BQUE_DEF_BUFF_SIZE_MAX      1024

/* get absolute difference of two unsigned integers. */
#define bque_abs_diff(a, b)         ((a) > (b) ? (a) - (b) : (b) - (a))

/**
 * @brief create a queue.
 * 
 * @param ctx the address of the context pointer.
 * @param conf configuration pointer.
*/
bque_res_t bque_new(bque_ctx_t **ctx, bque_conf_t *conf) {
    bque_ctx_t *alloc_ctx;

    BQUE_ASSERT(ctx != NULL);

    /* allocate context. */
    alloc_ctx = (bque_ctx_t *)malloc(sizeof(bque_ctx_t));
    if (alloc_ctx == NULL)
    {
        return BQUE_ERR_NO_MEM;
    }

    /* initialize context. */
    memset(alloc_ctx, 0, sizeof(bque_ctx_t));

    /* configure context. */
    if (conf != NULL) {
        alloc_ctx->conf.node_num_max = conf->buff_num_max;
        alloc_ctx->conf.buff_size_max = conf->buff_size_max;
        alloc_ctx->conf.free_buff_cb = conf->free_buff_cb;
    } else {
        alloc_ctx->conf.node_num_max = BQUE_DEF_NODE_NUM_MAX;
        alloc_ctx->conf.buff_size_max = BQUE_DEF_BUFF_SIZE_MAX;
        alloc_ctx->conf.free_buff_cb = NULL;
    }

    *ctx = alloc_ctx;

    return BQUE_OK;
}

/**
 * @brief delete the queue.
 * 
 * @param ctx context pointer.
*/
bque_res_t bque_del(bque_ctx_t *ctx) {
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
bque_res_t bque_stat(bque_ctx_t *ctx, bque_stat_t *stat) {
    BQUE_ASSERT(ctx != NULL);
    BQUE_ASSERT(stat != NULL);

    stat->buff_num = ctx->cache.node_num;

    return BQUE_OK;
}

/**
 * @brief create a new node.
 * 
 * @param node the address of the node pointer.
 * @param size buffer size.
*/
static bque_res_t create_node(bque_node_t **node, bque_u32_t size) {
    bque_node_t *alloc_node;
    bque_u8_t *alloc_buff;

    BQUE_ASSERT(node != NULL);

    /* allocate buffer. */
    alloc_buff = (bque_u8_t *)malloc(size);
    if (alloc_buff == NULL) {
        return BQUE_ERR_NO_MEM;
    }

    /* allocate node. */
    alloc_node = (bque_node_t *)malloc(sizeof(bque_node_t));
    if (alloc_node == NULL) {
        free(alloc_buff);

        return BQUE_ERR_NO_MEM;
    }

    /* initialize node. */
    memset(alloc_node, 0, sizeof(bque_node_t));
    alloc_node->buff = alloc_buff;
    alloc_node->size = size;

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
bque_res_t bque_enqueue(bque_ctx_t *ctx, const void *buff, bque_u32_t size) {
    bque_node_t *new_node;
    bque_res_t res;

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
        memcpy(new_node->buff, buff, size);
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
bque_res_t bque_preempt(bque_ctx_t *ctx, const void *buff, bque_u32_t size) {
    bque_node_t *new_node;
    bque_res_t res;

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
        memcpy(new_node->buff, buff, size);
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

    /* update the fast indexing cache. */
    if (ctx->cache.last.node != NULL) {
        ctx->cache.last.node_idx++;
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
bque_res_t bque_insert(bque_ctx_t *ctx, bque_u32_t idx, const void *buff, bque_u32_t size) {
    bque_node_t *new_node;
    bque_res_t res;
    bque_u32_t i;

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
        memcpy(new_node->buff, buff, size);
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
        bque_node_t *curt_node = ctx->head_node;

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

    /* update the fast indexing cache. */
    if (ctx->cache.last.node != NULL) {
        if (ctx->cache.last.node_idx >= idx) {
            ctx->cache.last.node_idx++;
        }
    }

    return BQUE_OK;
}

/**
 * @brief detach a buffer from the head of the queue.
 * 
 * @param ctx context pointer.
 * @param buff buffer pointer, when it's NULL, the buffer won't be copied.
 * @param size size pointer, when it's NULL, the size won't be copied.
*/
bque_res_t bque_dequeue(bque_ctx_t *ctx, void *buff, bque_u32_t *size) {
    BQUE_ASSERT(ctx != NULL);

    /* check whether the queue is empty. */
    if (ctx->cache.node_num == 0) {
        return BQUE_ERR_EMPTY_QUE;
    }

    /* if necessary, output the buffer and buffer size of the head node. */
    if (buff != NULL) {
        memcpy(buff, ctx->head_node->buff, ctx->head_node->size);
    }
    if (size != NULL) {
        *size = ctx->head_node->size;
    }

    /* remove the node. */
    if (ctx->cache.node_num == 1) {
        free(ctx->head_node->buff);
        free(ctx->head_node);
        ctx->head_node = NULL;
        ctx->tail_node = NULL;
        ctx->cache.node_num = 0;
    } else {
        bque_node_t *curt_node;

        curt_node = ctx->head_node;
        ctx->head_node = curt_node->next_node;
        ctx->head_node->prev_node = NULL;
        free(curt_node->buff);
        free(curt_node);
        ctx->cache.node_num--;
    }

    /* update the fast indexing cache. */
    if (ctx->cache.last.node != NULL) {
        if (ctx->cache.last.node_idx == 0) {
            ctx->cache.last.node = NULL;
            ctx->cache.last.node_idx = 0;
        } else {
            ctx->cache.last.node_idx--;
        }
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
bque_res_t bque_forfeit(bque_ctx_t *ctx, void *buff, bque_u32_t *size) {
    BQUE_ASSERT(ctx != NULL);

    /* check whether the queue is empty. */
    if (ctx->cache.node_num == 0) {
        return BQUE_ERR_EMPTY_QUE;
    }

    /* if necessary, output the buffer and buffer size of the head node. */
    if (buff != NULL) {
        memcpy(buff, ctx->tail_node->buff, ctx->tail_node->size);
    }
    if (size != NULL) {
        *size = ctx->tail_node->size;
    }

    /* remove the node. */
    if (ctx->cache.node_num == 1) {
        free(ctx->tail_node->buff);
        free(ctx->tail_node);
        ctx->head_node = NULL;
        ctx->tail_node = NULL;
        ctx->cache.node_num = 0;
    } else {
        bque_node_t *curt_node;

        curt_node = ctx->tail_node;
        ctx->tail_node = curt_node->prev_node;
        ctx->tail_node->next_node = NULL;
        free(curt_node->buff);
        free(curt_node);
        ctx->cache.node_num--;
    }

    /* update the fast indexing cache. */
    if (ctx->cache.last.node != NULL) {
        if (ctx->cache.last.node_idx == ctx->cache.node_num) {
            ctx->cache.last.node = NULL;
            ctx->cache.last.node_idx = 0;
        }
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
bque_res_t bque_drop(bque_ctx_t *ctx, bque_u32_t idx, void *buff, bque_u32_t *size) {
    bque_node_t *curt_node;
    bque_u32_t curt_idx;

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
        memcpy(buff, curt_node->buff, curt_node->size);
    }
    if (size != NULL) {
        *size = curt_node->size;
    }

    /* remove the node. */
    if (ctx->cache.node_num == 1) {
        free(curt_node->buff);
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
        free(curt_node->buff);
        free(curt_node);
        ctx->cache.node_num--;
    }

    /* update the fast indexing cache. */
    if (ctx->cache.last.node != NULL) {
        if (ctx->cache.last.node_idx == idx) {
            ctx->cache.last.node = NULL;
            ctx->cache.last.node_idx = 0;
        } else if (ctx->cache.last.node_idx > idx) {
            ctx->cache.last.node_idx--;
        }
    }

    return BQUE_OK;
}

/**
 * @brief empty the queue.
 * 
 * @param ctx context pointer.
*/
bque_res_t bque_empty(bque_ctx_t *ctx) {
    bque_node_t *curt_node;
    bque_node_t *next_node;

    BQUE_ASSERT(ctx != NULL);

    /* check whether the queue is empty. */
    if (ctx->cache.node_num == 0) {
        return BQUE_OK;
    }

    /* remove all nodes. */
    curt_node = ctx->head_node;
    if (ctx->conf.free_buff_cb != NULL) {
        bque_free_buff_cb_t free_buff_cb;

        free_buff_cb = ctx->conf.free_buff_cb;
        while (curt_node != NULL) {
            next_node = curt_node->next_node;
            free_buff_cb(curt_node->buff, curt_node->size);
            free(curt_node->buff);
            free(curt_node);
            curt_node = next_node;
        }
    } else {
        while (curt_node != NULL) {
            next_node = curt_node->next_node;
            free(curt_node->buff);
            free(curt_node);
            curt_node = next_node;
        }
    }

    /* reset context. */
    ctx->head_node = NULL;
    ctx->tail_node = NULL;
    ctx->cache.node_num = 0;

    /* update the fast indexing cache. */
    if (ctx->cache.last.node != NULL) {
        ctx->cache.last.node = NULL;
        ctx->cache.last.node_idx = 0;
    }

    return BQUE_OK;
}

/**
 * @brief Get buffer in the queue by index.
 * 
 * @note Parameters `buff` and `size` must not be NULL at the same time.
 * 
 * @param ctx The context pointer.
 * @param idx The buffer index, 0 and the positive value indicates forward
 *            indexing and the negative value indicates backward indexing.
 * @param buff The buffer pointer.
 * @param size The buffer size pointer.
 */
bque_res_t bque_item(bque_ctx_t *ctx, bque_s32_t idx,
                     void **buff, bque_size_t *size) {
    bque_iter_order_t iter_order = BQUE_ITER_FORWARD;
    bque_u32_t node_num;
    bque_u32_t node_idx_max;
    bque_u32_t forward_node_idx;
    bque_u32_t curt_node_idx;
    bque_u32_t temp_node_idx_diff;
    bque_u32_t node_idx_diff = 0xFFFFFFFF;
    bque_node_t *curt_node;

    BQUE_ASSERT(ctx != NULL);
    BQUE_ASSERT(buff != NULL ||
                size != NULL);

    /* check whether the index is valid. */
    node_num = ctx->cache.node_num;
    node_idx_max = node_num - 1;
    if (idx >= 0) {
        forward_node_idx = (bque_u32_t)idx;
        if (forward_node_idx > node_idx_max) {
            return BQUE_ERR_BAD_IDX;
        }
    } else {
        bque_u32_t temp;

        temp = (bque_u32_t)(-idx);
        if (temp > node_num) {
            return BQUE_ERR_BAD_IDX;
        }

        forward_node_idx = node_num - temp;
    }

    /* if fast indexing is available, check whether the last indexed node is
       closer. */
    if (ctx->cache.last.node != NULL) {
        if (forward_node_idx > ctx->cache.last.node_idx) {
            node_idx_diff = forward_node_idx - ctx->cache.last.node_idx;
            iter_order = BQUE_ITER_FORWARD;
        } else if (forward_node_idx < ctx->cache.last.node_idx) {
            node_idx_diff = ctx->cache.last.node_idx - forward_node_idx;
            iter_order = BQUE_ITER_BACKWARD;
        } else {
            if (buff != NULL) {
                *buff = ctx->cache.last.node->buff;
            }
            if (size != NULL) {
                *size = ctx->cache.last.node->size;
            }

            return BQUE_OK;
        }
        curt_node = ctx->cache.last.node;
        curt_node_idx = ctx->cache.last.node_idx;
    }

    /* check whether the head node is closer. */
    temp_node_idx_diff = bque_abs_diff(forward_node_idx, 0);
    if (temp_node_idx_diff < node_idx_diff) {
        node_idx_diff = temp_node_idx_diff;
        curt_node = ctx->head_node;
        curt_node_idx = 0;
        iter_order = BQUE_ITER_FORWARD;
    }

    /* check whether the tail node is closer. */
    temp_node_idx_diff = bque_abs_diff(forward_node_idx, node_idx_max);
    if (temp_node_idx_diff < node_idx_diff) {
        node_idx_diff = temp_node_idx_diff;
        curt_node = ctx->tail_node;
        curt_node_idx = node_idx_max;
        iter_order = BQUE_ITER_BACKWARD;
    }

    /* find the node indexed by forward_node_idx. */
    if (iter_order == BQUE_ITER_FORWARD) {
        while (curt_node_idx != forward_node_idx) {
            curt_node = curt_node->next_node;
            curt_node_idx++;
        }
    } else {
        while (curt_node_idx != forward_node_idx) {
            curt_node = curt_node->prev_node;
            curt_node_idx--;
        }
    }

    /* update the cache. */
    ctx->cache.last.node = curt_node;
    ctx->cache.last.node_idx = curt_node_idx;

    /* copy the buffer information. */
    if (buff != NULL) {
        *buff = curt_node->buff;
    }
    if (size != NULL) {
        *size = curt_node->size;
    }

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
bque_res_t bque_sort(bque_ctx_t *ctx, bque_sort_cb_t cb, bque_sort_order_t order) {
    bque_sort_res_t sort_res;
    bque_node_t **node_array;
    bque_node_t *curt_node;
    bque_node_t *temp_node;
    bque_u32_t node_num;
    bque_u32_t node_idx;

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
    node_array = (bque_node_t **)malloc(sizeof(bque_node_t *) * node_num);
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
        for (bque_u32_t i = 0; i < node_num - 1; i++) {
            for (bque_u32_t j = 0; j < node_num - i - 1; j++) {
                bque_node_t *node_a;
                bque_node_t *node_b;

                node_a = node_array[j];
                node_b = node_array[j + 1];
                sort_res = cb(node_a->buff, node_a->size, node_b->buff, node_b->size);
                if (sort_res == BQUE_SORT_GREATER) {
                    temp_node = node_array[j];
                    node_array[j] = node_array[j + 1];
                    node_array[j + 1] = temp_node;
                }
            }
        }
    } else {
        for (bque_u32_t i = 0; i < node_num - 1; i++) {
            for (bque_u32_t j = 0; j < node_num - i - 1; j++) {
                bque_node_t *node_a;
                bque_node_t *node_b;

                node_a = node_array[j];
                node_b = node_array[j + 1];
                sort_res = cb(node_a->buff, node_a->size, node_b->buff, node_b->size);
                if (sort_res == BQUE_SORT_LESS) {
                    temp_node = node_array[j];
                    node_array[j] = node_array[j + 1];
                    node_array[j + 1] = temp_node;
                }
            }
        }
    }

    /* link all the nodes in the array. */
    for (bque_u32_t i = 0; i < node_num; i++) {
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

    /* update the fast indexing cache. */
    if (ctx->cache.last.node != NULL) {
        ctx->cache.last.node = NULL;
        ctx->cache.last.node_idx = 0;
    }

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
bque_res_t bque_foreach(bque_ctx_t *ctx, bque_iter_cb_t cb, bque_iter_order_t order) {
    bque_u32_t node_num;
    bque_u32_t node_idx;
    bque_node_t *curt_node;
    bque_res_t res;

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
            res = cb(node_idx, node_num, curt_node->buff, curt_node->size);
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
            res = cb(node_idx, node_num, curt_node->buff, curt_node->size);
            if (res == BQUE_ERR_ITER_STOP) {
                return BQUE_ERR_ITER_STOP;
            }

            curt_node = curt_node->prev_node;
            node_idx--;
        } while (curt_node != NULL);
    }

    return BQUE_OK;
}

/**
 * @brief Get or set option within the buffer queue.
 * 
 * @param ctx The context pointer.
 * @param option The option.
 * @param arg Option argument.
 */
bque_res_t bque_option(bque_ctx_t *ctx, bque_option_t option, void *arg) {
    BQUE_ASSERT(ctx != NULL);

    switch (option) {
        case BQUE_OPT_GET_MAX_BUFF_NUM: {
            if (arg != NULL) {
                *(bque_size_t *)arg = ctx->conf.node_num_max;
            }
        } break;

        case BQUE_OPT_SET_MAX_BUFF_NUM: {
            if (arg != NULL) {
                ctx->conf.node_num_max = *(bque_size_t *)arg;
            }
        } break;

        case BQUE_OPT_GET_MAX_BUFF_SIZE: {
            if (arg != NULL) {
                *(bque_size_t *)arg = ctx->conf.buff_size_max;
            }
        } break;

        case BQUE_OPT_SET_MAX_BUFF_SIZE: {
            if (arg != NULL) {
                ctx->conf.buff_size_max = *(bque_size_t *)arg;
            }
        } break;

        case BQUE_OPT_SET_FREE_BUFF_CB: {
            ctx->conf.free_buff_cb = (bque_free_buff_cb_t)arg;
        } break;

        default: {
            return BQUE_ERR_BAD_OPT;
        } break;
    }

    return BQUE_OK;
}
