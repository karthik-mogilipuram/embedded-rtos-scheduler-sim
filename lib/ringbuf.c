#include "ringbuf.h"

void ringbuf_init(ringbuf_t *rb, uint8_t *storage, size_t capacity) {
    rb->data = storage;
    rb->capacity = capacity;
    rb->head = 0;
    rb->tail = 0;
    rb->size = 0;
}

void ringbuf_reset(ringbuf_t *rb) {
    rb->head = 0;
    rb->tail = 0;
    rb->size = 0;
}

size_t ringbuf_size(const ringbuf_t *rb) {
    return rb->size;
}

size_t ringbuf_capacity(const ringbuf_t *rb) {
    return rb->capacity;
}

bool ringbuf_is_empty(const ringbuf_t *rb) {
    return rb->size == 0;
}

bool ringbuf_is_full(const ringbuf_t *rb) {
    return rb->size == rb->capacity;
}

// We will implement these in the next step (after we confirm build is still clean)
size_t ringbuf_write(ringbuf_t *rb, const uint8_t *src, size_t n) {
    if (rb == NULL || src == NULL || n == 0 || rb->capacity == 0 || rb->data == NULL) {
        return 0;
    }

    size_t written = 0;

    for (size_t i = 0; i < n; i++) {
        // 1) Write the new byte at the current head position
        rb->data[rb->head] = src[i];

        // 2) Move head forward (wrap around using modulo)
        rb->head = (rb->head + 1) % rb->capacity;

        // 3) If buffer was full, overwrite oldest => move tail forward
        if (rb->size == rb->capacity) {
            rb->tail = (rb->tail + 1) % rb->capacity;
        } else {
            // otherwise, buffer grew by 1
            rb->size++;
        }

        written++;
    }

    return written;
}


size_t ringbuf_read(ringbuf_t *rb, uint8_t *dst, size_t n) {
    if (rb == NULL || dst == NULL || n == 0 || rb->capacity == 0 || rb->data == NULL) {
        return 0;
    }

    // You can't read more than what's currently stored
    size_t to_read = (n < rb->size) ? n : rb->size;

    for (size_t i = 0; i < to_read; i++) {
        // 1) Read the byte at the current tail position
        dst[i] = rb->data[rb->tail];

        // 2) Move tail forward (wrap around)
        rb->tail = (rb->tail + 1) % rb->capacity;

        // 3) Buffer now has one less byte
        rb->size--;
    }

    return to_read;
}


