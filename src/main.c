#include <stdio.h>
#include <stdint.h>
#include <pthread.h>
#include <windows.h>   // Sleep, GetTickCount64

#include "msg_queue.h"
#include "ringbuf.h"
#include "i2c_mock.h"
#include "i2c_util.h"

/*
  Read one register with retry.
  - final return value = final status (OK/TIMEOUT/NACK/...)
  - retries_used_out = how many retries were used (0..retries)
  - fail_* counters = counts of failed attempts that happened during retries
*/
static i2c_status_t i2c_read_reg_retry(uint8_t dev, uint8_t reg,
                                       uint8_t *buf, size_t len,
                                       uint32_t retries,
                                       int *retries_used_out,
                                       uint32_t *fail_timeout_out,
                                       uint32_t *fail_nack_out,
                                       uint32_t *fail_other_out)
{
    if (retries_used_out) *retries_used_out = 0;

    i2c_status_t st = I2C_ERR_IO;
    uint32_t attempts = 0;

    for (uint32_t attempt = 0; attempt <= retries; attempt++) {
        attempts++;

        st = i2c_mock_read(dev, reg, buf, len);

        if (st == I2C_OK) break;

        // Count attempt failures (even if later retry succeeds)
        if (st == I2C_ERR_TIMEOUT) {
            if (fail_timeout_out) (*fail_timeout_out)++;
        } else if (st == I2C_ERR_NACK) {
            if (fail_nack_out) (*fail_nack_out)++;
        } else {
            if (fail_other_out) (*fail_other_out)++;
        }

        // Retry only on recoverable errors
        if (!(st == I2C_ERR_TIMEOUT || st == I2C_ERR_NACK)) break;
    }

    if (retries_used_out) {
        *retries_used_out = (attempts > 0) ? (int)(attempts - 1) : 0;
    }

    return st;
}

// --- Thread args ---
typedef struct {
    msg_queue_t *q;
    int samples;
    int period_ms;

    uint8_t dev_addr;
    uint8_t reg_addr;
    uint32_t retries;

    uint32_t timeout_every;
    uint32_t nack_every;
} sensor_args_t;

typedef struct {
    msg_queue_t *q;
    ringbuf_t   *log_rb;

    // flush policy
    uint32_t flush_every_msgs;     // flush after N messages
    uint32_t flush_interval_ms;    // or after T ms
} logger_args_t;


// --- Sensor task: periodic sampling -> queue ---
static void* sensor_task(void* arg) {
    sensor_args_t *a = (sensor_args_t*)arg;

    // Enable fault injection for reads
    i2c_mock_set_timeout_every(a->timeout_every);
    i2c_mock_set_nack_every(a->nack_every);

    // Stable periodic timing (like vTaskDelayUntil)
    uint64_t next = (uint64_t)GetTickCount64();

    // Final outcome counters (what the app ends up with)
    uint32_t ok = 0, timeout = 0, nack = 0, other = 0;

    // Attempt-failure counters (what happened during retries)
    uint32_t fail_timeout = 0, fail_nack = 0, fail_other = 0;

    for (int i = 0; i < a->samples; i++) {
        // Update fake device register value reliably (no faults during write)
        i2c_mock_set_timeout_every(0);
        i2c_mock_set_nack_every(0);

        uint8_t write_val = (uint8_t)(100 + i);
        (void)i2c_mock_write(a->dev_addr, a->reg_addr, &write_val, 1);

        // Restore fault injection for the read path
        i2c_mock_set_timeout_every(a->timeout_every);
        i2c_mock_set_nack_every(a->nack_every);

        // Read with retry + capture attempt failures
        uint8_t read_val = 0;
        int retries_used = 0;

        i2c_status_t st = i2c_read_reg_retry(a->dev_addr, a->reg_addr,
                                             &read_val, 1,
                                             a->retries,
                                             &retries_used,
                                             &fail_timeout, &fail_nack, &fail_other);

        // Final outcome stats
        if (st == I2C_OK) ok++;
        else if (st == I2C_ERR_TIMEOUT) timeout++;
        else if (st == I2C_ERR_NACK) nack++;
        else other++;

        // Send to logger
        sample_msg_t msg;
        msg.type = MSG_DATA;
        msg.ts_ms = (uint64_t)GetTickCount64();
        msg.value = (st == I2C_OK) ? (int)read_val : -1;
        msg.status = (int)st;
        msg.retries = retries_used;

        msg_queue_push(a->q, msg);

        // Sleep until next tick
        next += (uint64_t)a->period_ms;
        uint64_t now = (uint64_t)GetTickCount64();
        if (next > now) Sleep((DWORD)(next - now));
    }

    // Stop logger
    sample_msg_t stop = {0};
    stop.type = MSG_STOP;
    msg_queue_push(a->q, stop);

    uint32_t fail_total = fail_timeout + fail_nack + fail_other;

    printf("[sensor_task] summary: final(ok=%u timeout=%u nack=%u other=%u)  "
           "attempt_fail(total=%u timeout=%u nack=%u other=%u)\n",
           ok, timeout, nack, other,
           fail_total, fail_timeout, fail_nack, fail_other);

    return NULL;
}

// --- Logger task: queue -> ring buffer -> flush (UART-like) ---
static void flush_ringbuf_to_stdout(ringbuf_t *rb) {
    uint8_t tmp[64];
    while (ringbuf_size(rb) > 0) {
        size_t r = ringbuf_read(rb, tmp, sizeof(tmp));
        if (r == 0) break;
        fwrite(tmp, 1, r, stdout);
    }
}

static void* logger_task(void* arg) {
    logger_args_t *a = (logger_args_t*)arg;
    uint32_t msgs_since_flush = 0;
    uint64_t last_flush_ms = (uint64_t)GetTickCount64();


    while (1) {
        sample_msg_t msg;
        msg_queue_pop(a->q, &msg);

        if (msg.type == MSG_STOP) {
            flush_ringbuf_to_stdout(a->log_rb);
            printf("[logger_task] received STOP\n");
            break;
        }

        const char *st_str = i2c_status_str((i2c_status_t)msg.status);

        char line[128];
        int len = snprintf(line, sizeof(line),
                           "[logger_task] t=%llu ms value=%d status=%s retries=%d\n",
                           (unsigned long long)msg.ts_ms,
                           msg.value,
                           st_str,
                           msg.retries);

        if (len < 0) len = 0;
        if (len > (int)sizeof(line)) len = (int)sizeof(line);

        ringbuf_write(a->log_rb, (const uint8_t*)line, (size_t)len);
        uint32_t msgs_since_flush = 0;
        uint64_t last_flush_ms = (uint64_t)GetTickCount64();
        msgs_since_flush++;

        uint64_t now = (uint64_t)GetTickCount64();
        int time_due = (now - last_flush_ms) >= a->flush_interval_ms;
        int msg_due  = msgs_since_flush >= a->flush_every_msgs;

        // Safety: if buffer is getting too full, flush now to reduce overwrite risk
        int buf_due  = ringbuf_size(a->log_rb) >= 200;

        if (time_due || msg_due || buf_due) {
            // Optional marker so you can SEE batching happening:
            // printf("[logger_task] FLUSH (msgs=%u, buf=%zu)\n", msgs_since_flush, ringbuf_size(a->log_rb));

            flush_ringbuf_to_stdout(a->log_rb);
            msgs_since_flush = 0;
            last_flush_ms = now;
        }

    }

    return NULL;
}

int main(void) {
    printf("Scheduler sim (threads + queue + I2C mock + ringbuf logger)\n");

    i2c_mock_reset_all();

    // Queue storage
    sample_msg_t q_storage[16];
    msg_queue_t q;
    if (!msg_queue_init(&q, q_storage, 16)) {
        printf("Queue init failed\n");
        return 1;
    }

    // Log ring buffer storage
    uint8_t log_storage[256];
    ringbuf_t log_rb;
    ringbuf_init(&log_rb, log_storage, sizeof(log_storage));

    pthread_t sensor_t, logger_t;

    // You can tweak these values for different demos
    sensor_args_t sargs = {
        .q = &q,
        .samples = 20,
        .period_ms = 200,

        .dev_addr = 0x48,
        .reg_addr = 0x10,
        .retries = 2,          // try 0,1,2

        .timeout_every = 5,    // try 0,5,7
        .nack_every = 7        // try 0,7,9
    };

    logger_args_t largs = {
    .q = &q,
    .log_rb = &log_rb,
    .flush_every_msgs = 5,
    .flush_interval_ms = 1000
};

    pthread_create(&logger_t, NULL, logger_task, &largs);
    pthread_create(&sensor_t, NULL, sensor_task, &sargs);

    pthread_join(sensor_t, NULL);
    pthread_join(logger_t, NULL);

    msg_queue_destroy(&q);

    printf("Scheduler sim done.\n");
    return 0;
}
