#ifndef I2C_H
#define I2C_H

#include <stdint.h>
#include <stdbool.h>

#define I2C_MIN_HZ 30418UL
#define I2C_MAX_HZ 400000UL

#ifndef I2C_BUFFER_SIZE
#define I2C_BUFFER_SIZE 16
#endif

#define I2C_OWNER_ANY 0

#define I2C_STATUS_BUS_ERROR 0x00
#define I2C_STATUS_START 0x08
#define I2C_STATUS_REP_START 0x10
#define I2C_STATUS_SLA_W_ACK 0x18
#define I2C_STATUS_SLA_W_NACK 0x20
#define I2C_STATUS_DATA_W_ACK 0x28
#define I2C_STATUS_DATA_W_NACK 0x30
#define I2C_STATUS_ARB_LOST 0x38
#define I2C_STATUS_SLA_R_ACK 0x40
#define I2C_STATUS_SLA_R_NACK 0x48
#define I2C_STATUS_DATA_R_ACK 0x50
#define I2C_STATUS_DATA_R_NACK 0x58
#define I2C_STATUS_NONE 0xF8

typedef enum {
    I2C_NONE,
    I2C_BUSY,
    I2C_ACK,
    I2C_NACK,
    I2C_OK,
    I2C_NACK_DATA,
    I2C_ARB_LOST,
    I2C_BUS_ERROR,
    I2C_TIMEOUT_START,
    I2C_TIMEOUT_ADDR,
    I2C_TIMEOUT_DATA,
    I2C_TIMEOUT_STOP,
    I2C_BAD_STATUS
} i2c_result_t;

typedef enum {
    I2C_PHASE_IDLE,
    I2C_PHASE_START,
    I2C_PHASE_ADDR,
    I2C_PHASE_TX,
    I2C_PHASE_REP_START,
    I2C_PHASE_ADDR_R,
    I2C_PHASE_RX,
    I2C_PHASE_STOP
} i2c_phase_t;

bool i2c_init(uint32_t Hz, bool pullup, uint32_t timeout_ms);

bool i2c_probe(uint8_t owner, uint8_t address, uint32_t now);
bool i2c_write(uint8_t owner, uint8_t address, const uint8_t *data, uint8_t length, uint32_t now);
bool i2c_read(uint8_t owner, uint8_t address, uint8_t reg, uint8_t length, uint32_t now);
i2c_result_t i2c_update(uint8_t owner, uint32_t now);
uint8_t i2c_get_data(uint8_t owner, uint8_t *data, uint8_t max);

bool i2c_probe_begin(uint8_t address, uint32_t now);
i2c_result_t i2c_probe_update(uint32_t now);
void i2c_reset(void);

bool i2c_is_busy(void);
uint8_t i2c_get_owner(void);
i2c_phase_t i2c_get_phase(void);
i2c_phase_t i2c_get_fail_phase(void);
uint8_t i2c_get_status(void);
uint8_t i2c_address_byte(uint8_t address, bool read);
bool i2c_sda_high(void);
bool i2c_scl_high(void);
uint32_t i2c_get_hz(void);

#endif
