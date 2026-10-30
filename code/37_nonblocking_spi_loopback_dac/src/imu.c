#include "imu.h"
#include "config.h"
#include "mpu6050.h"
#include "usart.h"

#include <avr/pgmspace.h>

static imu_state_t state = IMU_STOPPED;
static bool oneshot = false;
static bool stop_request = false;
static bool in_flight = false;
static uint32_t state_ms = 0;
static uint32_t sample_ms = 0;

static uint8_t id_value = 0;
static bool id_read = false;
static imu_error_t error = IMU_ERROR_NONE;
static i2c_result_t last_result = I2C_NONE;
static i2c_phase_t last_phase = I2C_PHASE_IDLE;
static uint8_t last_status = I2C_STATUS_NONE;
static uint8_t consecutive = 0;
static uint32_t errors = 0;
static uint32_t samples = 0;

static bool have_data = false;
static int16_t accel[3];
static uint32_t data_ms = 0;

typedef struct {
    imu_state_t state;
    bool oneshot;
    bool stopping;
    uint8_t id;
    bool id_read;
    imu_error_t error;
    i2c_result_t result;
    i2c_phase_t phase;
    uint8_t status;
    uint8_t consecutive;
    uint32_t errors;
    uint32_t samples;
    bool have_data;
    bool fresh;
    int16_t accel[3];
    uint32_t age;
} imu_report_t;

static imu_report_t snap;
static bool report_pending = false;

static imu_state_t event_state = IMU_STOPPED;
static bool event_pending = false;
static imu_error_t event_error = IMU_ERROR_NONE;
static i2c_result_t event_result = I2C_NONE;
static i2c_phase_t event_phase = I2C_PHASE_IDLE;
static uint8_t event_id = 0;

static void imu_event(imu_state_t new_state)
{
    event_state = new_state;
    event_error = error;
    event_result = last_result;
    event_phase = last_phase;
    event_id = id_value;
    event_pending = true;
}

static void imu_enter(imu_state_t new_state, uint32_t now)
{
    state = new_state;
    state_ms = now;
}

static void imu_write_reg(uint8_t reg, uint8_t value, uint32_t now)
{
    uint8_t data[2];

    data[0] = reg;
    data[1] = value;

    if(i2c_write(CONFIG_I2C_OWNER_IMU, CONFIG_IMU_ADDRESS, data, 2, now)){
        in_flight = true;
    }
}

static void imu_read_reg(uint8_t reg, uint8_t length, uint32_t now)
{
    if(i2c_read(CONFIG_I2C_OWNER_IMU, CONFIG_IMU_ADDRESS, reg, length, now)){
        in_flight = true;
    }
}

static void imu_fail(imu_error_t new_error, uint32_t now)
{
    error = new_error;
    errors++;
    consecutive++;

    if(consecutive > CONFIG_IMU_RETRIES){
        imu_enter(IMU_FAILED, now);
        imu_event(IMU_FAILED);
        return;
    }

    imu_enter(IMU_BACKOFF, now);
    imu_event(IMU_BACKOFF);
}

static void imu_handle(i2c_result_t result, uint32_t now)
{
    uint8_t data[MPU6050_ACCEL_BYTES];

    last_result = result;
    last_status = i2c_get_status();
    last_phase = i2c_get_fail_phase();

    if(result != I2C_OK){
        if(result == I2C_NACK && state == IMU_ID){
            imu_fail(IMU_ERROR_NO_DEVICE, now);
        } else {
            imu_fail(IMU_ERROR_BUS, now);
        }
        return;
    }

    switch(state){
        case IMU_ID:
            if(i2c_get_data(CONFIG_I2C_OWNER_IMU, data, sizeof(data)) != 1){
                imu_fail(IMU_ERROR_BUS, now);
                return;
            }
            id_value = data[0];
            id_read = true;

            if(id_value != MPU6050_WHO_AM_I_VALUE){
                error = IMU_ERROR_ID;
                errors++;
                imu_enter(IMU_ID_ERROR, now);
                imu_event(IMU_ID_ERROR);
                return;
            }
            imu_enter(IMU_RESET, now);
            return;

        case IMU_RESET:
            imu_enter(IMU_RESET_WAIT, now);
            return;

        case IMU_WAKE:
            imu_enter(IMU_CONFIG, now);
            return;

        case IMU_CONFIG:
            imu_enter(IMU_ACCEL_CONFIG, now);
            return;

        case IMU_ACCEL_CONFIG:
            imu_enter(IMU_VERIFY_CONFIG, now);
            return;

        case IMU_VERIFY_CONFIG:
            if(i2c_get_data(CONFIG_I2C_OWNER_IMU, data, sizeof(data)) != 3 || !mpu6050_check_config(data, CONFIG_IMU_DLPF, CONFIG_IMU_ACCEL_FS)){
                imu_fail(IMU_ERROR_CONFIG, now);
                return;
            }
            imu_enter(IMU_VERIFY_POWER, now);
            return;

        case IMU_VERIFY_POWER:
            if(i2c_get_data(CONFIG_I2C_OWNER_IMU, data, sizeof(data)) != 1 || (data[0] & 0xC7) != MPU6050_PWR_CLOCK_PLL_X){
                imu_fail(IMU_ERROR_CONFIG, now);
                return;
            }
            sample_ms = now - CONFIG_IMU_PERIOD_MS;
            imu_enter(IMU_READY, now);
            imu_event(IMU_READY);
            return;

        case IMU_SAMPLE:
            if(i2c_get_data(CONFIG_I2C_OWNER_IMU, data, sizeof(data)) != MPU6050_ACCEL_BYTES){
                imu_fail(IMU_ERROR_BUS, now);
                return;
            }
            mpu6050_parse_accel(data, accel);
            have_data = true;
            data_ms = now;
            samples++;
            consecutive = 0;
            error = IMU_ERROR_NONE;

            if(oneshot){
                oneshot = false;
                imu_enter(IMU_STOPPED, now);
                imu_event(IMU_STOPPED);
            } else {
                imu_enter(IMU_READY, now);
            }
            return;

        default:
            return;
    }
}

void imu_init(void)
{
    state = IMU_STOPPED;
    oneshot = false;
    stop_request = false;
    in_flight = false;
    have_data = false;
    id_read = false;
    error = IMU_ERROR_NONE;
    last_result = I2C_NONE;
    consecutive = 0;
    errors = 0;
    samples = 0;
    report_pending = false;
    event_pending = false;
}

void imu_update(uint32_t now)
{
    if(in_flight){
        i2c_result_t result = i2c_update(CONFIG_I2C_OWNER_IMU, now);

        if(result == I2C_BUSY || result == I2C_NONE){
            return;
        }

        in_flight = false;
        imu_handle(result, now);

        if(stop_request){
            stop_request = false;
            oneshot = false;

            if(state != IMU_FAILED && state != IMU_ID_ERROR && state != IMU_STOPPED){
                imu_enter(IMU_STOPPED, now);
                imu_event(IMU_STOPPED);
            }
        }
        return;
    }

    switch(state){
        case IMU_ID:
            imu_read_reg(MPU6050_REG_WHO_AM_I, 1, now);
            break;

        case IMU_RESET:
            imu_write_reg(MPU6050_REG_PWR_MGMT_1, MPU6050_PWR_RESET, now);
            break;

        case IMU_RESET_WAIT:
            if(now - state_ms >= CONFIG_IMU_RESET_MS){
                imu_enter(IMU_WAKE, now);
            }
            break;

        case IMU_WAKE:
            imu_write_reg(MPU6050_REG_PWR_MGMT_1, MPU6050_PWR_CLOCK_PLL_X, now);
            break;

        case IMU_CONFIG:
            imu_write_reg(MPU6050_REG_CONFIG, mpu6050_config_value(CONFIG_IMU_DLPF), now);
            break;

        case IMU_ACCEL_CONFIG:
            imu_write_reg(MPU6050_REG_ACCEL_CONFIG, mpu6050_accel_config_value(CONFIG_IMU_ACCEL_FS), now);
            break;

        case IMU_VERIFY_CONFIG:
            imu_read_reg(MPU6050_REG_CONFIG, 3, now);
            break;

        case IMU_VERIFY_POWER:
            imu_read_reg(MPU6050_REG_PWR_MGMT_1, 1, now);
            break;

        case IMU_READY:
            if(now - sample_ms >= CONFIG_IMU_PERIOD_MS){
                imu_enter(IMU_SAMPLE, now);
                imu_read_reg(MPU6050_REG_ACCEL_XOUT_H, MPU6050_ACCEL_BYTES, now);

                if(in_flight){
                    sample_ms = now;
                }
            }
            break;

        case IMU_SAMPLE:
            imu_read_reg(MPU6050_REG_ACCEL_XOUT_H, MPU6050_ACCEL_BYTES, now);

            if(in_flight){
                sample_ms = now;
            }
            break;

        case IMU_BACKOFF:
            if(now - state_ms >= CONFIG_IMU_RETRY_MS){
                imu_enter(IMU_ID, now);
            }
            break;

        default:
            break;
    }
}

bool imu_is_active(void)
{
    return state != IMU_STOPPED && state != IMU_FAILED && state != IMU_ID_ERROR;
}

static void imu_begin(bool single, uint32_t now)
{
    oneshot = single;
    stop_request = false;
    id_read = false;
    id_value = 0;
    consecutive = 0;
    error = IMU_ERROR_NONE;
    imu_enter(IMU_ID, now);
}

bool imu_start(uint32_t now)
{
    if(imu_is_active()){
        return false;
    }

    imu_begin(false, now);
    return true;
}

bool imu_read_once(uint32_t now)
{
    if(imu_is_active()){
        return false;
    }

    imu_begin(true, now);
    return true;
}

bool imu_stop(uint32_t now)
{
    if(!imu_is_active()){
        return false;
    }

    if(in_flight){
        stop_request = true;
    } else {
        oneshot = false;
        imu_enter(IMU_STOPPED, now);
        imu_event(IMU_STOPPED);
    }
    return true;
}

imu_state_t imu_get_state(void)
{
    return state;
}

bool imu_stopping(void)
{
    return stop_request;
}

imu_error_t imu_get_error(void)
{
    return error;
}

i2c_result_t imu_get_last_result(void)
{
    return last_result;
}

i2c_phase_t imu_get_last_phase(void)
{
    return last_phase;
}

uint8_t imu_get_id(void)
{
    return id_value;
}

bool imu_has_data(void)
{
    return have_data;
}

bool imu_is_fresh(uint32_t now)
{
    return have_data && now - data_ms <= CONFIG_IMU_STALE_MS;
}

void imu_get_accel(int16_t out[3])
{
    for(uint8_t i = 0; i < 3; i++){
        out[i] = accel[i];
    }
}

uint32_t imu_get_samples(void)
{
    return samples;
}

uint32_t imu_get_errors(void)
{
    return errors;
}

uint8_t imu_get_consecutive(void)
{
    return consecutive;
}

uint32_t imu_get_data_ms(void)
{
    return data_ms;
}

void imu_request_report(uint32_t now)
{
    snap.state = state;
    snap.oneshot = oneshot;
    snap.stopping = stop_request;
    snap.id = id_value;
    snap.id_read = id_read;
    snap.error = error;
    snap.result = last_result;
    snap.phase = last_phase;
    snap.status = last_status;
    snap.consecutive = consecutive;
    snap.errors = errors;
    snap.samples = samples;
    snap.have_data = have_data;
    snap.fresh = imu_is_fresh(now);
    snap.age = now - data_ms;

    for(uint8_t i = 0; i < 3; i++){
        snap.accel[i] = accel[i];
    }

    report_pending = true;
}

bool imu_report_pending(void)
{
    return report_pending;
}

static void imu_send_hex(uint8_t value)
{
    static const char digits[] PROGMEM = "0123456789ABCDEF";
    char text[5];

    text[0] = '0';
    text[1] = 'x';
    text[2] = pgm_read_byte(&digits[value >> 4]);
    text[3] = pgm_read_byte(&digits[value & 0x0F]);
    text[4] = '\0';
    usart0_send_string(text);
}

static void imu_send_int32(int32_t value)
{
    if(value < 0){
        usart0_send_string_P(PSTR("-"));
        usart0_send_uint32((uint32_t)(-value));
    } else {
        usart0_send_uint32((uint32_t)value);
    }
}

static void imu_send_state(imu_state_t value)
{
    if(value == IMU_STOPPED){
        usart0_send_string_P(PSTR("STOPPED"));
    } else if(value == IMU_READY || value == IMU_SAMPLE){
        usart0_send_string_P(PSTR("RUNNING"));
    } else if(value == IMU_BACKOFF){
        usart0_send_string_P(PSTR("RETRY_WAIT"));
    } else if(value == IMU_FAILED){
        usart0_send_string_P(PSTR("FAILED"));
    } else if(value == IMU_ID_ERROR){
        usart0_send_string_P(PSTR("ID_ERROR"));
    } else {
        usart0_send_string_P(PSTR("INIT"));
    }
}

static void imu_send_result(i2c_result_t result, i2c_phase_t phase)
{
    static const char names[][14] PROGMEM = {
        "NONE", "BUSY", "ACK", "ADDR_NACK", "OK", "DATA_NACK", "ARB_LOST",
        "BUS_ERROR", "TIMEOUT_START", "TIMEOUT_ADDR", "TIMEOUT_DATA", "TIMEOUT_STOP", "BAD_STATUS"
    };
    static const char phases[][10] PROGMEM = {
        "IDLE", "START", "ADDR_W", "TX", "REP_START", "ADDR_R", "RX", "STOP"
    };

    if(result <= I2C_BAD_STATUS){
        usart0_send_string_P(names[result]);
    }

    if(result != I2C_OK && result != I2C_NONE && phase <= I2C_PHASE_STOP){
        usart0_send_string_P(PSTR(" in "));
        usart0_send_string_P(phases[phase]);
    }
}

static void imu_send_error(imu_error_t value)
{
    if(value == IMU_ERROR_NONE){
        usart0_send_string_P(PSTR("none"));
    } else if(value == IMU_ERROR_NO_DEVICE){
        usart0_send_string_P(PSTR("NO_DEVICE (address NACK)"));
    } else if(value == IMU_ERROR_ID){
        usart0_send_string_P(PSTR("ID_MISMATCH"));
    } else if(value == IMU_ERROR_CONFIG){
        usart0_send_string_P(PSTR("CONFIG_READBACK"));
    } else {
        usart0_send_string_P(PSTR("BUS"));
    }
}

void imu_send_report(void)
{
    report_pending = false;

    usart0_send_string_P(PSTR("IMU MPU6050 @"));
    imu_send_hex(CONFIG_IMU_ADDRESS);
    usart0_send_string_P(PSTR(" (AD0 "));
    usart0_send_uint32(CONFIG_IMU_AD0);
    usart0_send_string_P(PSTR("): "));
    imu_send_state(snap.state);

    if(snap.stopping){
        usart0_send_string_P(PSTR(" (stopping)"));
    }
    if(snap.oneshot){
        usart0_send_string_P(PSTR(" (single read)"));
    }

    usart0_send_string_P(PSTR(", WHO_AM_I "));

    if(snap.id_read){
        imu_send_hex(snap.id);
        if(snap.id == MPU6050_WHO_AM_I_VALUE){
            usart0_send_string_P(PSTR(" OK"));
        } else {
            usart0_send_string_P(PSTR(" expected 0x68"));
        }
    } else {
        usart0_send_string_P(PSTR("-"));
    }
    usart0_send_newline();

    usart0_send_string_P(PSTR("IMU ACCEL "));

    if(snap.have_data){
        for(uint8_t i = 0; i < 3; i++){
            imu_send_int32(snap.accel[i]);
            usart0_send_string_P(PSTR(" "));
        }
        usart0_send_string_P(PSTR("raw, "));
        for(uint8_t i = 0; i < 3; i++){
            imu_send_int32(mpu6050_accel_mg(snap.accel[i], CONFIG_IMU_ACCEL_FS));
            usart0_send_string_P(PSTR(" "));
        }
        usart0_send_string_P(PSTR("mg (+-"));
        usart0_send_uint32(mpu6050_accel_range_g(CONFIG_IMU_ACCEL_FS));
        usart0_send_string_P(PSTR(" g, "));
        usart0_send_uint32(mpu6050_accel_lsb_per_g(CONFIG_IMU_ACCEL_FS));
        usart0_send_string_P(PSTR(" LSB/g), AGE "));
        usart0_send_uint32(snap.age);

        if(snap.fresh){
            usart0_send_string_P(PSTR(" ms FRESH"));
        } else {
            usart0_send_string_P(PSTR(" ms STALE"));
        }
    } else {
        usart0_send_string_P(PSTR("none"));
    }
    usart0_send_newline();

    usart0_send_string_P(PSTR("IMU SAMPLES "));
    usart0_send_uint32(snap.samples);
    usart0_send_string_P(PSTR(", ERRORS "));
    usart0_send_uint32(snap.errors);
    usart0_send_string_P(PSTR(" (in a row "));
    usart0_send_uint32(snap.consecutive);
    usart0_send_string_P(PSTR("/"));
    usart0_send_uint32(CONFIG_IMU_RETRIES);
    usart0_send_string_P(PSTR("), LAST "));
    imu_send_error(snap.error);
    usart0_send_string_P(PSTR(", I2C "));
    imu_send_result(snap.result, snap.phase);
    usart0_send_string_P(PSTR(", TWSR "));
    imu_send_hex(snap.status);
    usart0_send_newline();

    usart0_send_string_P(PSTR("IMU BUS "));
    usart0_send_uint32(i2c_get_hz() / 1000);
    usart0_send_string_P(PSTR(" kHz, internal pull-up "));

    if(CONFIG_I2C_PULLUP){
        usart0_send_string_P(PSTR("on"));
    } else {
        usart0_send_string_P(PSTR("off"));
    }

    if(CONFIG_I2C_MODULE_PULLUP){
        usart0_send_string_P(PSTR(", module pull-ups expected"));
    }
    if(CONFIG_I2C_LEVEL_SHIFT_CHECK){
        usart0_send_string_P(PSTR(", check 3.3 V / 5 V levels"));
    }
    usart0_send_newline();
}

bool imu_event_pending(void)
{
    return event_pending;
}

void imu_send_event(void)
{
    event_pending = false;

    usart0_send_string_P(PSTR("IMU "));
    imu_send_state(event_state);

    if(event_state == IMU_READY){
        usart0_send_string_P(PSTR(", WHO_AM_I "));
        imu_send_hex(event_id);
    } else if(event_state != IMU_STOPPED){
        usart0_send_string_P(PSTR(", "));
        imu_send_error(event_error);

        if(event_error == IMU_ERROR_ID){
            usart0_send_string_P(PSTR(" got "));
            imu_send_hex(event_id);
            usart0_send_string_P(PSTR(" expected 0x68"));
        } else {
            usart0_send_string_P(PSTR(", I2C "));
            imu_send_result(event_result, event_phase);
        }
    }
    usart0_send_newline();
}
