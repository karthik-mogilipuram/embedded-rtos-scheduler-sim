#ifndef RINGBUF_H
#define RINGBUF_H

#include <stddef.h>
#include <stdint.h>
#include <stdbool.h>

typedef struct {
    uint8_t *data;      // memory storage (external array)
    size_t   capacity;  // total size of data[]
    size_t   head;      // next write index
    size_t   tail;      // next read index
    size_t   size;      // number of bytes currently stored
} ringbuf_t;

// Initialize ring buffer with user-provided storage
void   ringbuf_init(ringbuf_t *rb, uint8_t *storage, size_t capacity);

// Reset to empty (keeps same storage/capacity)
void   ringbuf_reset(ringbuf_t *rb);

// Info helpers
size_t ringbuf_size(const ringbuf_t *rb);
size_t ringbuf_capacity(const ringbuf_t *rb);
bool   ringbuf_is_empty(const ringbuf_t *rb);
bool   ringbuf_is_full(const ringbuf_t *rb);

// Write/read bytes
// Write behavior when full: overwrite oldest bytes (log-friendly)
size_t ringbuf_write(ringbuf_t *rb, const uint8_t *src, size_t n);
size_t ringbuf_read(ringbuf_t *rb, uint8_t *dst, size_t n);

#endif // RINGBUF_H
