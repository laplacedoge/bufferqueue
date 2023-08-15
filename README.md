# bufferqueue
A library for manipulating linked buffers within a queue.

# What you can do with it?
- Use `bque_new()` to create a new buffer queue.
- Use `bque_enqueue()` to add a buffer to the end of the queue.
- Use `bque_dequeue()` to remove a buffer from the beginning of the queue.
- Use `bque_stat()` to get the status information of a buffer queue.
- Use `bque_empty()` to remove all buffers from the queue.
- Use `bque_free()` to free a buffer queue.

# What you can EVEN do with it?
- Use `bque_preempt()` to add a buffer to the beginning of the queue.
- Use `bque_forfeit()` to remove a buffer from the end of the queue.
- Use `bque_item()` to get the buffer at a specific position in the queue.
- Use `bque_insert()` to add a buffer to the queue at a specific position.
- Use `bque_drop()` to remove a buffer from the queue at a specific position.

# And sure it can also...
- Use `bque_sort()` to sort the buffers in the queue using your own sorting rule.
- Use `bque_foreach()` to iterate through the buffers in the queue forwardly or backwardly.
