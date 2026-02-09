#include "i2c_mock.h"
#include <string.h>

#define MAX_ADDR 128
#define REG_SIZE 256

static uint8_t  dev_regs[MAX_ADDR][REG_SIZE];
static uint32_t op_count = 0;
static uint32_t timeout_every = 0;
static uint32_t nack_every = 0;

void i2c_mock_reset_all(void) {
    memset(dev_regs, 0, sizeof(dev_regs));
    op_count = 0;
    timeout_every = 0;
    nack_every = 0;
}

void i2c_mock_reset_device(uint8_t dev_addr) {
    if (dev_addr < MAX_ADDR) {
        memset(dev_regs[dev_addr], 0, REG_SIZE);
    }
}

void i2c_mock_set_timeout_every(uint32_t n) { timeout_every = n; }
void i2c_mock_set_nack_every(uint32_t n)    { nack_every = n; }

static i2c_status_t maybe_fail(void) {
    op_count++;

    if (timeout_every != 0 && (op_count % timeout_every) == 0) return I2C_ERR_TIMEOUT;
    if (nack_every != 0 && (op_count % nack_every) == 0)       return I2C_ERR_NACK;

    return I2C_OK;
}

i2c_status_t i2c_mock_read(uint8_t dev_addr, uint8_t reg, uint8_t *buf, size_t len) {
    if (buf == NULL || len == 0) return I2C_ERR_INVALID_ARG;
    if (dev_addr >= MAX_ADDR) return I2C_ERR_INVALID_ARG;

    i2c_status_t f = maybe_fail();
    if (f != I2C_OK) return f;

    if ((size_t)reg + len > REG_SIZE) return I2C_ERR_INVALID_ARG;
    memcpy(buf, &dev_regs[dev_addr][reg], len);
    return I2C_OK;
}

i2c_status_t i2c_mock_write(uint8_t dev_addr, uint8_t reg, const uint8_t *data, size_t len) {
    if (data == NULL || len == 0) return I2C_ERR_INVALID_ARG;
    if (dev_addr >= MAX_ADDR) return I2C_ERR_INVALID_ARG;

    i2c_status_t f = maybe_fail();
    if (f != I2C_OK) return f;

    if ((size_t)reg + len > REG_SIZE) return I2C_ERR_INVALID_ARG;
    memcpy(&dev_regs[dev_addr][reg], data, len);
    return I2C_OK;
}
