#include "i2c_util.h"

void i2c_stats_reset(i2c_stats_t *s) {
    if (!s) return;
    s->ok = s->timeout = s->nack = s->invalid = s->io = 0;
}

const char* i2c_status_str(i2c_status_t st) {
    switch (st) {
        case I2C_OK: return "OK";
        case I2C_ERR_INVALID_ARG: return "INVALID_ARG";
        case I2C_ERR_NACK: return "NACK";
        case I2C_ERR_TIMEOUT: return "TIMEOUT";
        case I2C_ERR_IO: return "IO";
        default: return "UNKNOWN";
    }
}

static void stats_add(i2c_stats_t *s, i2c_status_t st) {
    if (!s) return;
    switch (st) {
        case I2C_OK: s->ok++; break;
        case I2C_ERR_TIMEOUT: s->timeout++; break;
        case I2C_ERR_NACK: s->nack++; break;
        case I2C_ERR_INVALID_ARG: s->invalid++; break;
        default: s->io++; break;
    }
}

// Laptop simulation rule: timeout_ms == 0 means “immediate timeout”
static i2c_status_t check_timeout(uint32_t timeout_ms) {
    return (timeout_ms == 0) ? I2C_ERR_TIMEOUT : I2C_OK;
}

i2c_status_t i2c_read_reg(uint8_t dev_addr, uint8_t reg, uint8_t *buf, size_t len,
                          uint32_t timeout_ms, uint32_t retries, i2c_stats_t *stats) {
    if (buf == NULL || len == 0) {
        stats_add(stats, I2C_ERR_INVALID_ARG);
        return I2C_ERR_INVALID_ARG;
    }
    if (check_timeout(timeout_ms) != I2C_OK) {
        stats_add(stats, I2C_ERR_TIMEOUT);
        return I2C_ERR_TIMEOUT;
    }

    i2c_status_t st = I2C_ERR_IO;

    for (uint32_t attempt = 0; attempt <= retries; attempt++) {
        st = i2c_mock_read(dev_addr, reg, buf, len);
        if (st == I2C_OK) break;

        // Only retry on timeout or nack
        if (!(st == I2C_ERR_TIMEOUT || st == I2C_ERR_NACK)) break;
    }

    stats_add(stats, st);
    return st;
}

i2c_status_t i2c_write_reg(uint8_t dev_addr, uint8_t reg, const uint8_t *data, size_t len,
                           uint32_t timeout_ms, uint32_t retries, i2c_stats_t *stats) {
    if (data == NULL || len == 0) {
        stats_add(stats, I2C_ERR_INVALID_ARG);
        return I2C_ERR_INVALID_ARG;
    }
    if (check_timeout(timeout_ms) != I2C_OK) {
        stats_add(stats, I2C_ERR_TIMEOUT);
        return I2C_ERR_TIMEOUT;
    }

    i2c_status_t st = I2C_ERR_IO;

    for (uint32_t attempt = 0; attempt <= retries; attempt++) {
        st = i2c_mock_write(dev_addr, reg, data, len);
        if (st == I2C_OK) break;

        if (!(st == I2C_ERR_TIMEOUT || st == I2C_ERR_NACK)) break;
    }

    stats_add(stats, st);
    return st;
}
