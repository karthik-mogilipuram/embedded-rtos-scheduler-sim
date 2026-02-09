#include "msg_queue.h"

bool msg_queue_init(msg_queue_t *q, sample_msg_t *storage, size_t capacity) {
    if (!q || !storage || capacity == 0) return false;

    q->buf = storage;
    q->capacity = capacity;
    q->head = 0;
    q->tail = 0;
    q->count = 0;

    pthread_mutex_init(&q->mtx, NULL);
    pthread_cond_init(&q->not_empty, NULL);
    pthread_cond_init(&q->not_full, NULL);

    return true;
}

void msg_queue_destroy(msg_queue_t *q) {
    if (!q) return;
    pthread_mutex_destroy(&q->mtx);
    pthread_cond_destroy(&q->not_empty);
    pthread_cond_destroy(&q->not_full);
}

bool msg_queue_push(msg_queue_t *q, sample_msg_t item) {
    if (!q) return false;

    pthread_mutex_lock(&q->mtx);

    // Wait until there is space
    while (q->count == q->capacity) {
        pthread_cond_wait(&q->not_full, &q->mtx);
    }

    // Write item at head
    q->buf[q->head] = item;
    q->head = (q->head + 1) % q->capacity;
    q->count++;

    // Signal that queue is not empty now
    pthread_cond_signal(&q->not_empty);
    pthread_mutex_unlock(&q->mtx);
    return true;
}

bool msg_queue_pop(msg_queue_t *q, sample_msg_t *out) {
    if (!q || !out) return false;

    pthread_mutex_lock(&q->mtx);

    // Wait until there is something to read
    while (q->count == 0) {
        pthread_cond_wait(&q->not_empty, &q->mtx);
    }

    // Read item at tail
    *out = q->buf[q->tail];
    q->tail = (q->tail + 1) % q->capacity;
    q->count--;

    // Signal that queue is not full now
    pthread_cond_signal(&q->not_full);
    pthread_mutex_unlock(&q->mtx);
    return true;
}
