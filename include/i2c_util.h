#ifndef I2C_UTIL_H
#define I2C_UTIL_H

#include <stddef.h>
#include <stdint.h>
#include "i2c_mock.h"

typedef struct {
    uint32_t ok;
    uint32_t timeout;
    uint32_t nack;
    uint32_t invalid;
    uint32_t io;
} i2c_stats_t;

void i2c_stats_reset(i2c_stats_t *s);
const char* i2c_status_str(i2c_status_t st);

// Driver-style APIs (what you’d use in “real” embedded code)
i2c_status_t i2c_read_reg(uint8_t dev_addr, uint8_t reg, uint8_t *buf, size_t len,
                          uint32_t timeout_ms, uint32_t retries, i2c_stats_t *stats);

i2c_status_t i2c_write_reg(uint8_t dev_addr, uint8_t reg, const uint8_t *data, size_t len,
                           uint32_t timeout_ms, uint32_t retries, i2c_stats_t *stats);

#endif
