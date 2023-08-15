#include <stdio.h>
#include <getopt.h>
#include <stdlib.h>

#include "bufferqueue.h"

#define NSORT_EOL       "\n"

#define NSORT_OPT_STR   "ad"

#define NSORT_HELP_STR  "nsort [-a] [-d] num0 num1 num2 ... numN"   NSORT_EOL   \
                        "Sorts the given numbers."                  NSORT_EOL   \
                        "Options:"                                  NSORT_EOL   \
                        "  -a  Sort in ascending order (default)"   NSORT_EOL   \
                        "  -d  Sort in descending order"            NSORT_EOL

static bque_sort_order_t sort_order = BQUE_SORT_ASCENDING;

static bque_sort_res_t num_sort_cb(const void *buff_a, bque_size_t size_a,
                                   const void *buff_b, bque_size_t size_b) {
    long int num_a;
    long int num_b;

    num_a = *(long int *)buff_a;
    num_b = *(long int *)buff_b;

    if (num_a < num_b) {
        return BQUE_SORT_LESS;
    } else if (num_a > num_b) {
        return BQUE_SORT_GREATER;
    } else {
        return BQUE_SORT_EQUAL;
    }
}

static bque_res_t num_iter_cb(bque_u32_t idx, bque_u32_t num,
                              void *buff, bque_size_t size) {
    long int num_val;

    num_val = *(long int *)buff;

    if (idx == 0) {
        if (sort_order == BQUE_SORT_ASCENDING) {
            printf("Sorted numbers in ascending order:" NSORT_EOL);
        } else {
            printf("Sorted numbers in descending order:" NSORT_EOL);
        }
        if (num == 1) {
            printf("%ld", num_val);
        } else {
            printf("%ld ", num_val);
        }
    }
    else if (idx != num - 1) {
        printf("%ld ", num_val);
    } else {
        printf("%ld" NSORT_EOL, num_val);
    }

    return BQUE_OK;
}

int main(int argc, char **argv) {
    bque_ctx_t *ctx;
    bque_res_t res;
    int num_num;
    int opt;
    int arg;

    /* Parse the command line arguments. */
    while ((opt = getopt(argc, argv, NSORT_OPT_STR)) != -1) {
        switch (opt) {
            case 'a': {
                sort_order = BQUE_SORT_ASCENDING;
            } break;

            case 'd': {
                sort_order = BQUE_SORT_DESCENDING;
            } break;

            default: {
                fprintf(stderr, NSORT_HELP_STR);
                exit(EXIT_FAILURE);
            } break;
        }
    }

    /* Check whether there are any numbers to sort. */
    num_num = argc - optind;
    if (num_num <= 0) {
        fprintf(stderr, NSORT_HELP_STR);
        exit(EXIT_FAILURE);
    }

    /* Create a new buffer queue. */
    res = bque_new(&ctx, NULL);
    if (res != BQUE_OK) {
        goto error_exit;
    }

    /* Configure the buffer queue. */

    /* No limit on the number of buffers. */
    arg = 0;
    res = bque_option(ctx, BQUE_OPT_SET_MAX_BUFF_NUM, &arg);
    if (res != BQUE_OK) {
        goto free_bque;
    }

    /* Limit the size of each buffer to the size of a long int. */
    arg = (int)sizeof(long int);
    res = bque_option(ctx, BQUE_OPT_SET_MAX_BUFF_SIZE, &arg);
    if (res != BQUE_OK) {
        goto free_bque;
    }

    for (int i = optind; i < argc; i++) {
        long int num;
        char *endptr;

        num = strtol(argv[i], &endptr, 10);
        if (*endptr != '\0') {
            fprintf(stderr, "Invalid number: %s\n", argv[i]);
            goto free_bque;
        }

        /* Add the number to the buffer queue. */
        res = bque_enqueue(ctx, &num, sizeof(num));
        if (res != BQUE_OK) {
            goto free_bque;
        }
    }

    /* Sort the numbers. */
    res = bque_sort(ctx, num_sort_cb, sort_order);
    if (res != BQUE_OK) {
        goto free_bque;
    }

    /* Iterate over the sorted numbers and print them. */
    res = bque_foreach(ctx, num_iter_cb, BQUE_ITER_FORWARD);
    if (res != BQUE_OK) {
        goto free_bque;
    }

    /* Free the buffer queue. */
    bque_free(ctx);

    return EXIT_SUCCESS;

free_bque:
    bque_free(ctx);

error_exit:
    return EXIT_FAILURE;
}
