#include "i2c.h"
#include <avr/io.h>
#include <util/atomic.h>

typedef enum {
    I2C_MODE_PROBE,
    I2C_MODE_WRITE,
    I2C_MODE_READ
} i2c_mode_t;

static i2c_phase_t i2c_phase = I2C_PHASE_IDLE;
static i2c_phase_t i2c_fail_phase = I2C_PHASE_IDLE;
static i2c_result_t i2c_pending = I2C_NONE;
static i2c_mode_t i2c_mode = I2C_MODE_PROBE;
static uint8_t i2c_owner = I2C_OWNER_ANY;
static uint8_t i2c_address = 0;
static uint8_t i2c_reg = 0;
static uint8_t i2c_tx[I2C_BUFFER_SIZE];
static uint8_t i2c_tx_length = 0;
static uint8_t i2c_tx_pos = 0;
static uint8_t i2c_rx[I2C_BUFFER_SIZE];
static uint8_t i2c_rx_length = 0;
static uint8_t i2c_rx_pos = 0;
static bool i2c_rx_complete = false;
static bool i2c_rx_ok = false;
static uint8_t i2c_status = I2C_STATUS_NONE;
static uint32_t i2c_phase_ms = 0;
static uint32_t i2c_timeout = 5;
static uint32_t i2c_hz = 0;

static uint8_t i2c_read_status(void)
{
    return TWSR & 0xF8;
}

static void i2c_enter(i2c_phase_t phase, uint32_t now)
{
    i2c_phase = phase;
    i2c_phase_ms = now;
}

static void i2c_finish(i2c_result_t result)
{
    if(result != I2C_ACK && result != I2C_OK){
        i2c_fail_phase = i2c_phase;
    }

    i2c_pending = result;
    i2c_phase = I2C_PHASE_IDLE;
}

static void i2c_send_stop(i2c_result_t result, uint32_t now)
{
    if(result != I2C_ACK && result != I2C_OK){
        i2c_fail_phase = i2c_phase;
    }

    TWCR = (1 << TWINT) | (1 << TWSTO) | (1 << TWEN);
    i2c_pending = result;
    i2c_enter(I2C_PHASE_STOP, now);
}

static void i2c_fail(uint8_t status, uint32_t now)
{
    if(status == I2C_STATUS_ARB_LOST){
        TWCR = (1 << TWINT) | (1 << TWEN);
        i2c_finish(I2C_ARB_LOST);
    } else if(status == I2C_STATUS_BUS_ERROR){
        i2c_send_stop(I2C_BUS_ERROR, now);
    } else {
        i2c_fail_phase = i2c_phase;
        i2c_reset();
        i2c_pending = I2C_BAD_STATUS;
    }
}

static i2c_result_t i2c_timeout_result(void)
{
    if(i2c_phase == I2C_PHASE_START || i2c_phase == I2C_PHASE_REP_START){
        return I2C_TIMEOUT_START;
    }
    if(i2c_phase == I2C_PHASE_ADDR || i2c_phase == I2C_PHASE_ADDR_R){
        return I2C_TIMEOUT_ADDR;
    }
    if(i2c_phase == I2C_PHASE_STOP){
        return I2C_TIMEOUT_STOP;
    }
    return I2C_TIMEOUT_DATA;
}

static bool i2c_begin(uint8_t owner, i2c_mode_t mode, uint8_t address, uint32_t now)
{
    if(i2c_phase != I2C_PHASE_IDLE || i2c_pending != I2C_NONE || address > 0x7F){
        return false;
    }

    i2c_owner = owner;
    i2c_mode = mode;
    i2c_address = address;
    i2c_status = I2C_STATUS_NONE;
    i2c_fail_phase = I2C_PHASE_IDLE;
    i2c_rx_complete = false;
    i2c_rx_ok = false;
    i2c_tx_pos = 0;
    i2c_rx_pos = 0;
    TWCR = (1 << TWINT) | (1 << TWSTA) | (1 << TWEN);
    i2c_enter(I2C_PHASE_START, now);
    return true;
}

bool i2c_init(uint32_t Hz, bool pullup, uint32_t timeout_ms)
{
    if(Hz < I2C_MIN_HZ || Hz > I2C_MAX_HZ || timeout_ms == 0){
        return false;
    }

    uint32_t twbr = (F_CPU - 16UL * Hz + 2UL * Hz - 1) / (2UL * Hz);

    if(twbr < 10 || twbr > 255){
        return false;
    }

    uint32_t actual = F_CPU / (16UL + 2UL * twbr);

    if(actual > Hz || actual < I2C_MIN_HZ){
        return false;
    }

    ATOMIC_BLOCK(ATOMIC_RESTORESTATE){
        DDRD &= ~((1 << DDD0) | (1 << DDD1));

        if(pullup){
            PORTD |= (1 << PORTD0) | (1 << PORTD1);
        } else {
            PORTD &= ~((1 << PORTD0) | (1 << PORTD1));
        }
    }

    TWCR = 0;
    TWSR = 0;
    TWBR = (uint8_t)twbr;
    TWCR = (1 << TWEN);

    i2c_hz = actual;
    i2c_timeout = timeout_ms;
    i2c_phase = I2C_PHASE_IDLE;
    i2c_pending = I2C_NONE;
    i2c_status = I2C_STATUS_NONE;
    return true;
}

bool i2c_probe(uint8_t owner, uint8_t address, uint32_t now)
{
    if(i2c_phase != I2C_PHASE_IDLE || i2c_pending != I2C_NONE || address > 0x7F){
        return false;
    }

    i2c_tx_length = 0;
    i2c_rx_length = 0;
    return i2c_begin(owner, I2C_MODE_PROBE, address, now);
}

bool i2c_write(uint8_t owner, uint8_t address, const uint8_t *data, uint8_t length, uint32_t now)
{
    if(length == 0 || length > I2C_BUFFER_SIZE || data == 0){
        return false;
    }

    if(i2c_phase != I2C_PHASE_IDLE || i2c_pending != I2C_NONE || address > 0x7F){
        return false;
    }

    for(uint8_t i = 0; i < length; i++){
        i2c_tx[i] = data[i];
    }

    i2c_tx_length = length;
    i2c_rx_length = 0;
    return i2c_begin(owner, I2C_MODE_WRITE, address, now);
}

bool i2c_read(uint8_t owner, uint8_t address, uint8_t reg, uint8_t length, uint32_t now)
{
    if(length == 0 || length > I2C_BUFFER_SIZE){
        return false;
    }

    if(i2c_phase != I2C_PHASE_IDLE || i2c_pending != I2C_NONE || address > 0x7F){
        return false;
    }

    i2c_reg = reg;
    i2c_tx_length = 0;
    i2c_rx_length = length;
    return i2c_begin(owner, I2C_MODE_READ, address, now);
}

static void i2c_step(uint8_t status, uint32_t now)
{
    switch(i2c_phase){
        case I2C_PHASE_START:
            if(status != I2C_STATUS_START){
                i2c_fail(status, now);
                return;
            }
            TWDR = i2c_address_byte(i2c_address, false);
            TWCR = (1 << TWINT) | (1 << TWEN);
            i2c_enter(I2C_PHASE_ADDR, now);
            return;

        case I2C_PHASE_ADDR:
            if(status == I2C_STATUS_SLA_W_NACK){
                i2c_send_stop(I2C_NACK, now);
                return;
            }
            if(status != I2C_STATUS_SLA_W_ACK){
                i2c_fail(status, now);
                return;
            }
            if(i2c_mode == I2C_MODE_PROBE){
                i2c_send_stop(I2C_ACK, now);
                return;
            }
            if(i2c_mode == I2C_MODE_READ){
                TWDR = i2c_reg;
            } else {
                TWDR = i2c_tx[0];
                i2c_tx_pos = 1;
            }
            TWCR = (1 << TWINT) | (1 << TWEN);
            i2c_enter(I2C_PHASE_TX, now);
            return;

        case I2C_PHASE_TX:
            if(status == I2C_STATUS_DATA_W_NACK){
                i2c_send_stop(I2C_NACK_DATA, now);
                return;
            }
            if(status != I2C_STATUS_DATA_W_ACK){
                i2c_fail(status, now);
                return;
            }
            if(i2c_mode == I2C_MODE_READ){
                TWCR = (1 << TWINT) | (1 << TWSTA) | (1 << TWEN);
                i2c_enter(I2C_PHASE_REP_START, now);
                return;
            }
            if(i2c_tx_pos < i2c_tx_length){
                TWDR = i2c_tx[i2c_tx_pos];
                i2c_tx_pos++;
                TWCR = (1 << TWINT) | (1 << TWEN);
                i2c_enter(I2C_PHASE_TX, now);
                return;
            }
            i2c_send_stop(I2C_OK, now);
            return;

        case I2C_PHASE_REP_START:
            if(status != I2C_STATUS_REP_START){
                i2c_fail(status, now);
                return;
            }
            TWDR = i2c_address_byte(i2c_address, true);
            TWCR = (1 << TWINT) | (1 << TWEN);
            i2c_enter(I2C_PHASE_ADDR_R, now);
            return;

        case I2C_PHASE_ADDR_R:
            if(status == I2C_STATUS_SLA_R_NACK){
                i2c_send_stop(I2C_NACK, now);
                return;
            }
            if(status != I2C_STATUS_SLA_R_ACK){
                i2c_fail(status, now);
                return;
            }
            if(i2c_rx_length > 1){
                TWCR = (1 << TWINT) | (1 << TWEA) | (1 << TWEN);
            } else {
                TWCR = (1 << TWINT) | (1 << TWEN);
            }
            i2c_enter(I2C_PHASE_RX, now);
            return;

        case I2C_PHASE_RX:
            if(status == I2C_STATUS_DATA_R_ACK && i2c_rx_pos + 1 < i2c_rx_length){
                i2c_rx[i2c_rx_pos] = TWDR;
                i2c_rx_pos++;

                if(i2c_rx_length - i2c_rx_pos > 1){
                    TWCR = (1 << TWINT) | (1 << TWEA) | (1 << TWEN);
                } else {
                    TWCR = (1 << TWINT) | (1 << TWEN);
                }
                i2c_enter(I2C_PHASE_RX, now);
                return;
            }
            if(status == I2C_STATUS_DATA_R_NACK && i2c_rx_pos + 1 == i2c_rx_length){
                i2c_rx[i2c_rx_pos] = TWDR;
                i2c_rx_pos++;
                i2c_rx_complete = true;
                i2c_send_stop(I2C_OK, now);
                return;
            }
            i2c_fail(status, now);
            return;

        default:
            return;
    }
}

i2c_result_t i2c_update(uint8_t owner, uint32_t now)
{
    i2c_result_t result = I2C_NONE;

    if(owner != i2c_owner){
        return I2C_NONE;
    }

    if(i2c_phase == I2C_PHASE_STOP){
        if(TWCR & (1 << TWSTO)){
            if(now - i2c_phase_ms > i2c_timeout){
                i2c_fail_phase = I2C_PHASE_STOP;
                i2c_reset();
                i2c_pending = I2C_TIMEOUT_STOP;
                i2c_rx_complete = false;
            } else {
                return I2C_BUSY;
            }
        } else {
            i2c_phase = I2C_PHASE_IDLE;
            i2c_rx_ok = i2c_rx_complete && i2c_pending == I2C_OK;
        }
    } else if(i2c_phase != I2C_PHASE_IDLE){
        if(!(TWCR & (1 << TWINT))){
            if(now - i2c_phase_ms <= i2c_timeout){
                return I2C_BUSY;
            }
            result = i2c_timeout_result();
            i2c_fail_phase = i2c_phase;
            i2c_reset();
            i2c_pending = result;
        } else {
            i2c_status = i2c_read_status();
            i2c_step(i2c_status, now);
        }
    }

    if(i2c_phase != I2C_PHASE_IDLE){
        return I2C_BUSY;
    }

    result = i2c_pending;
    i2c_pending = I2C_NONE;
    return result;
}

uint8_t i2c_get_data(uint8_t owner, uint8_t *data, uint8_t max)
{
    if(owner != i2c_owner || i2c_phase != I2C_PHASE_IDLE || !i2c_rx_ok || data == 0 || max < i2c_rx_length){
        return 0;
    }

    for(uint8_t i = 0; i < i2c_rx_length; i++){
        data[i] = i2c_rx[i];
    }
    return i2c_rx_length;
}

bool i2c_probe_begin(uint8_t address, uint32_t now)
{
    return i2c_probe(I2C_OWNER_ANY, address, now);
}

i2c_result_t i2c_probe_update(uint32_t now)
{
    return i2c_update(I2C_OWNER_ANY, now);
}

void i2c_reset(void)
{
    TWCR = 0;
    TWCR = (1 << TWEN);
    i2c_phase = I2C_PHASE_IDLE;
}

bool i2c_is_busy(void)
{
    return i2c_phase != I2C_PHASE_IDLE || i2c_pending != I2C_NONE;
}

uint8_t i2c_get_owner(void)
{
    return i2c_owner;
}

i2c_phase_t i2c_get_phase(void)
{
    return i2c_phase;
}

i2c_phase_t i2c_get_fail_phase(void)
{
    return i2c_fail_phase;
}

uint8_t i2c_get_status(void)
{
    return i2c_status;
}

uint8_t i2c_address_byte(uint8_t address, bool read)
{
    return (uint8_t)((address << 1) | (read ? 1 : 0));
}

bool i2c_sda_high(void)
{
    return (PIND & (1 << PIND1)) != 0;
}

bool i2c_scl_high(void)
{
    return (PIND & (1 << PIND0)) != 0;
}

uint32_t i2c_get_hz(void)
{
    return i2c_hz;
}
