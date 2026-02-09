#ifndef I2C_MOCK_H
#define I2C_MOCK_H

#include <stddef.h>
#include <stdint.h>

typedef enum {
    I2C_OK = 0,
    I2C_ERR_INVALID_ARG,
    I2C_ERR_NACK,
    I2C_ERR_TIMEOUT,
    I2C_ERR_IO
} i2c_status_t;

// Fake device memory + helpers
void i2c_mock_reset_all(void);
void i2c_mock_reset_device(uint8_t dev_addr);

// Fault injection (deterministic)
// If set to N, every Nth operation returns that error.
void i2c_mock_set_timeout_every(uint32_t n);
void i2c_mock_set_nack_every(uint32_t n);

// Low-level mock operations
i2c_status_t i2c_mock_read(uint8_t dev_addr, uint8_t reg, uint8_t *buf, size_t len);
i2c_status_t i2c_mock_write(uint8_t dev_addr, uint8_t reg, const uint8_t *data, size_t len);

#endif
