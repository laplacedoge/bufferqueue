# Table of contents
- [Table of contents](#table-of-contents)
- [Introduction](#introduction)
- [Abilities](#abilities)
  - [What you can do with it?](#what-you-can-do-with-it)
  - [What you can EVEN do with it?](#what-you-can-even-do-with-it)
  - [And sure it can also...](#and-sure-it-can-also)
- [Usage](#usage)
  - [Create a bque context](#create-a-bque-context)
  - [Configure your context](#configure-your-context)
  - [Free your context](#free-your-context)

# Introduction
A library for manipulating linked buffers within a queue.

# Abilities

## What you can do with it?
- Use `bque_new()` to create a new buffer queue.
- Use `bque_enqueue()` to add a buffer to the end of the queue.
- Use `bque_dequeue()` to remove a buffer from the beginning of the queue.
- Use `bque_stat()` to get the status information of a buffer queue.
- Use `bque_empty()` to remove all buffers from the queue.
- Use `bque_free()` to free a buffer queue.

## What you can EVEN do with it?
- Use `bque_preempt()` to add a buffer to the beginning of the queue.
- Use `bque_forfeit()` to remove a buffer from the end of the queue.
- Use `bque_item()` to get the buffer at a specific position in the queue.
- Use `bque_insert()` to add a buffer to the queue at a specific position.
- Use `bque_drop()` to remove a buffer from the queue at a specific position.

## And sure it can also...
- Use `bque_sort()` to sort the buffers in the queue using your own sorting rule.
- Use `bque_foreach()` to iterate through the buffers in the queue forwardly or backwardly.

# Usage

## Create a bque context
```c
bque_ctx *ctx;
bque_res res;

res = bque_new(&ctx, NULL);
if (res != BQUE_OK) {
    /* Failed to create the bque context.
       Check the value of `res` for details. */
}
```

## Configure your context
```c
/* Unlimited number of buffers. */
int max_buff_num = 0;

/* Maximum buffer size of 1K bytes. */
int max_buff_size = 1024;

bque_adjust(ctx, BQUE_OPT_SET_MAX_BUFF_NUM, &max_buff_num);

bque_adjust(ctx, BQUE_OPT_SET_MAX_BUFF_SIZE, &max_buff_size);
```

## Free your context
```c
bque_free(ctx);
```
