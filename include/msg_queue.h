#ifndef MSG_QUEUE_H
#define MSG_QUEUE_H

#include <stddef.h>
#include <stdint.h>
#include <pthread.h>
#include <stdbool.h>

// A message that "sensor_task" sends to "logger_task"
typedef enum {
    MSG_DATA = 0,
    MSG_STOP = 1
} msg_type_t;

typedef struct {
    msg_type_t type;
    uint64_t   ts_ms;     // timestamp in ms (from system uptime)
    int        value;     // simulated sensor value
    int        status;    // 0=OK, nonzero=error (later we’ll map to I2C errors)
    int        retries;   // how many retries were used (later)
} sample_msg_t;

// A fixed-size blocking queue for sample_msg_t
typedef struct {
    sample_msg_t *buf;
    size_t capacity;
    size_t head;     // next write
    size_t tail;     // next read
    size_t count;

    pthread_mutex_t mtx;
    pthread_cond_t  not_empty;
    pthread_cond_t  not_full;
} msg_queue_t;

bool   msg_queue_init(msg_queue_t *q, sample_msg_t *storage, size_t capacity);
void   msg_queue_destroy(msg_queue_t *q);

// Blocking push/pop
bool   msg_queue_push(msg_queue_t *q, sample_msg_t item);
bool   msg_queue_pop(msg_queue_t *q, sample_msg_t *out);

#endif
