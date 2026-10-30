#include "scanner.h"
#include "config.h"
#include "i2c.h"
#include "usart.h"

#include <avr/pgmspace.h>

static scanner_state_t state = SCANNER_IDLE;
static scanner_fault_t fault = SCANNER_FAULT_NONE;
static bool probing = false;
static bool cancel = false;
static uint8_t address = CONFIG_I2C_FIRST;
static uint8_t retries_now = 0;
static bool low_seen = false;
static uint32_t low_ms = 0;
static uint32_t start_ms = 0;
static uint32_t end_ms = 0;

static uint8_t found[16];
static uint8_t found_total = 0;
static uint32_t counts[I2C_BAD_STATUS + 1];
static uint32_t retries = 0;
static uint8_t last_status = I2C_STATUS_NONE;

typedef struct {
    scanner_state_t state;
    scanner_fault_t fault;
    bool cancel;
    bool sda;
    bool scl;
    uint8_t address;
    uint8_t status;
    uint8_t total;
    uint8_t map[16];
    uint32_t counts[I2C_BAD_STATUS + 1];
    uint32_t retries;
    uint32_t duration;
    uint32_t hz;
} scanner_report_t;

static scanner_report_t snap;
static bool head_pending = false;
static bool list_pending = false;
static uint8_t print_address = 0;

static void scanner_end(scanner_state_t end_state, scanner_fault_t end_fault, uint32_t now)
{
    state = end_state;
    fault = end_fault;
    probing = false;
    cancel = false;
    end_ms = now;
    scanner_request_report(now);
}

static scanner_fault_t scanner_line_fault(void)
{
    bool sda = i2c_sda_high();
    bool scl = i2c_scl_high();

    if(!sda && !scl){
        return SCANNER_FAULT_BOTH_LOW;
    }
    if(!sda){
        return SCANNER_FAULT_SDA_LOW;
    }
    if(!scl){
        return SCANNER_FAULT_SCL_LOW;
    }
    return SCANNER_FAULT_NONE;
}

static scanner_fault_t scanner_result_fault(i2c_result_t result)
{
    if(result == I2C_ARB_LOST){
        return SCANNER_FAULT_ARB_LOST;
    }
    if(result == I2C_BUS_ERROR){
        return SCANNER_FAULT_BUS_ERROR;
    }
    if(result == I2C_BAD_STATUS){
        return SCANNER_FAULT_BAD_STATUS;
    }
    return SCANNER_FAULT_TIMEOUT;
}

static void scanner_next(uint32_t now)
{
    retries_now = 0;

    if(address >= CONFIG_I2C_LAST){
        scanner_end(SCANNER_DONE, SCANNER_FAULT_NONE, now);
        return;
    }

    address++;
}

static void scanner_handle(i2c_result_t result, uint32_t now)
{
    probing = false;
    counts[result]++;
    last_status = i2c_get_status();

    if(result == I2C_ACK){
        found[address >> 3] |= (uint8_t)(1 << (address & 7));
        found_total++;
        scanner_next(now);
    } else if(result == I2C_NACK){
        scanner_next(now);
    } else {
        retries_now++;

        if(retries_now > CONFIG_I2C_RETRIES){
            scanner_fault_t line = scanner_line_fault();
            scanner_end(SCANNER_FAULT, line != SCANNER_FAULT_NONE ? line : scanner_result_fault(result), now);
            return;
        }

        retries++;
    }

    if(state == SCANNER_RUNNING && cancel){
        scanner_end(SCANNER_CANCELLED, SCANNER_FAULT_NONE, now);
    }
}

void scanner_init(void)
{
    state = SCANNER_IDLE;
    fault = SCANNER_FAULT_NONE;
    probing = false;
    cancel = false;
    head_pending = false;
    list_pending = false;
    found_total = 0;

    for(uint8_t i = 0; i < 16; i++){
        found[i] = 0;
    }
}

void scanner_update(uint32_t now)
{
    if(state != SCANNER_RUNNING){
        return;
    }

    if(probing){
        i2c_result_t result = i2c_probe_update(now);

        if(result != I2C_BUSY && result != I2C_NONE){
            scanner_handle(result, now);
        }
        return;
    }

    if(cancel){
        scanner_end(SCANNER_CANCELLED, SCANNER_FAULT_NONE, now);
        return;
    }

    scanner_fault_t line = scanner_line_fault();

    if(line != SCANNER_FAULT_NONE){
        if(!low_seen){
            low_seen = true;
            low_ms = now;
        } else if(now - low_ms > CONFIG_I2C_STUCK_MS){
            scanner_end(SCANNER_FAULT, line, now);
        }
        return;
    }

    low_seen = false;

    if(i2c_probe_begin(address, now)){
        probing = true;
    } else {
        i2c_reset();
    }
}

bool scanner_start(uint32_t now)
{
    if(state == SCANNER_RUNNING){
        return false;
    }

    if(i2c_is_busy()){
        i2c_reset();
    }

    for(uint8_t i = 0; i < 16; i++){
        found[i] = 0;
    }

    for(uint8_t i = 0; i <= I2C_BAD_STATUS; i++){
        counts[i] = 0;
    }

    found_total = 0;
    retries = 0;
    retries_now = 0;
    last_status = I2C_STATUS_NONE;
    address = CONFIG_I2C_FIRST;
    fault = SCANNER_FAULT_NONE;
    probing = false;
    cancel = false;
    start_ms = now;
    end_ms = now;
    low_seen = false;
    state = SCANNER_RUNNING;
    return true;
}

bool scanner_stop(uint32_t now)
{
    if(state != SCANNER_RUNNING){
        return false;
    }

    if(!probing){
        scanner_end(SCANNER_CANCELLED, SCANNER_FAULT_NONE, now);
    } else {
        cancel = true;
    }
    return true;
}

scanner_state_t scanner_get_state(void)
{
    return state;
}

scanner_fault_t scanner_get_fault(void)
{
    return fault;
}

bool scanner_cancelling(void)
{
    return state == SCANNER_RUNNING && cancel;
}

uint8_t scanner_get_address(void)
{
    return address;
}

bool scanner_found(uint8_t addr)
{
    if(addr > 0x7F){
        return false;
    }
    return (found[addr >> 3] & (1 << (addr & 7))) != 0;
}

uint8_t scanner_found_count(void)
{
    return found_total;
}

uint32_t scanner_get_count(uint8_t result)
{
    if(result > I2C_BAD_STATUS){
        return 0;
    }
    return counts[result];
}

uint32_t scanner_get_retries(void)
{
    return retries;
}

uint32_t scanner_get_duration(uint32_t now)
{
    if(state == SCANNER_RUNNING){
        return now - start_ms;
    }
    return end_ms - start_ms;
}

void scanner_request_report(uint32_t now)
{
    snap.state = state;
    snap.fault = fault;
    snap.cancel = cancel;
    snap.sda = i2c_sda_high();
    snap.scl = i2c_scl_high();
    snap.address = address;
    snap.status = last_status;
    snap.total = found_total;
    snap.retries = retries;
    snap.duration = scanner_get_duration(now);
    snap.hz = i2c_get_hz();

    for(uint8_t i = 0; i < 16; i++){
        snap.map[i] = found[i];
    }

    for(uint8_t i = 0; i <= I2C_BAD_STATUS; i++){
        snap.counts[i] = counts[i];
    }

    print_address = CONFIG_I2C_FIRST;
    head_pending = true;
    list_pending = false;
}

bool scanner_head_pending(void)
{
    return head_pending;
}

static void scanner_send_hex(uint8_t value)
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

static void scanner_send_state(void)
{
    if(snap.state == SCANNER_IDLE){
        usart0_send_string_P(PSTR("IDLE"));
    } else if(snap.state == SCANNER_RUNNING){
        if(snap.cancel){
            usart0_send_string_P(PSTR("CANCELLING"));
        } else {
            usart0_send_string_P(PSTR("RUNNING"));
        }
    } else if(snap.state == SCANNER_DONE){
        usart0_send_string_P(PSTR("DONE"));
    } else if(snap.state == SCANNER_CANCELLED){
        usart0_send_string_P(PSTR("CANCELLED"));
    } else {
        usart0_send_string_P(PSTR("FAULT "));

        if(snap.fault == SCANNER_FAULT_SDA_LOW){
            usart0_send_string_P(PSTR("SDA_LOW"));
        } else if(snap.fault == SCANNER_FAULT_SCL_LOW){
            usart0_send_string_P(PSTR("SCL_LOW"));
        } else if(snap.fault == SCANNER_FAULT_BOTH_LOW){
            usart0_send_string_P(PSTR("SDA_SCL_LOW"));
        } else if(snap.fault == SCANNER_FAULT_ARB_LOST){
            usart0_send_string_P(PSTR("ARB_LOST"));
        } else if(snap.fault == SCANNER_FAULT_BUS_ERROR){
            usart0_send_string_P(PSTR("BUS_ERROR"));
        } else if(snap.fault == SCANNER_FAULT_BAD_STATUS){
            usart0_send_string_P(PSTR("BAD_STATUS"));
        } else {
            usart0_send_string_P(PSTR("TIMEOUT"));
        }
    }
}

void scanner_send_head(void)
{
    head_pending = false;
    list_pending = true;

    usart0_send_string_P(PSTR("I2C SCAN: "));
    scanner_send_state();
    usart0_send_string_P(PSTR(", ADDR "));
    scanner_send_hex(snap.address);
    usart0_send_string_P(PSTR(" ("));

    if(snap.state == SCANNER_DONE){
        usart0_send_uint32(CONFIG_I2C_LAST - CONFIG_I2C_FIRST + 1);
    } else {
        usart0_send_uint32(snap.address - CONFIG_I2C_FIRST);
    }
    usart0_send_string_P(PSTR("/"));
    usart0_send_uint32(CONFIG_I2C_LAST - CONFIG_I2C_FIRST + 1);
    usart0_send_string_P(PSTR(" done), "));
    usart0_send_uint32(snap.duration);
    usart0_send_string_P(PSTR(" ms, "));
    usart0_send_uint32(snap.hz / 1000);
    usart0_send_string_P(PSTR(" kHz"));
    usart0_send_newline();

    usart0_send_string_P(PSTR("I2C ACK "));
    usart0_send_uint32(snap.counts[I2C_ACK]);
    usart0_send_string_P(PSTR(", NACK "));
    usart0_send_uint32(snap.counts[I2C_NACK]);
    usart0_send_string_P(PSTR(", TIMEOUT start/addr/stop "));
    usart0_send_uint32(snap.counts[I2C_TIMEOUT_START]);
    usart0_send_string_P(PSTR("/"));
    usart0_send_uint32(snap.counts[I2C_TIMEOUT_ADDR]);
    usart0_send_string_P(PSTR("/"));
    usart0_send_uint32(snap.counts[I2C_TIMEOUT_STOP]);
    usart0_send_string_P(PSTR(", BUS_ERR "));
    usart0_send_uint32(snap.counts[I2C_BUS_ERROR]);
    usart0_send_string_P(PSTR(", ARB "));
    usart0_send_uint32(snap.counts[I2C_ARB_LOST]);
    usart0_send_string_P(PSTR(", BAD "));
    usart0_send_uint32(snap.counts[I2C_BAD_STATUS]);
    usart0_send_string_P(PSTR(", RETRY "));
    usart0_send_uint32(snap.retries);
    usart0_send_newline();

    usart0_send_string_P(PSTR("I2C TWSR "));
    scanner_send_hex(snap.status);

    if(snap.sda){
        usart0_send_string_P(PSTR(", SDA high"));
    } else {
        usart0_send_string_P(PSTR(", SDA LOW"));
    }

    if(snap.scl){
        usart0_send_string_P(PSTR(", SCL high"));
    } else {
        usart0_send_string_P(PSTR(", SCL LOW"));
    }

    usart0_send_string_P(PSTR(", 7-bit addresses (write byte = addr << 1), FOUND "));
    usart0_send_uint32(snap.total);
    usart0_send_newline();
}

bool scanner_list_pending(void)
{
    return list_pending;
}

void scanner_send_list_line(void)
{
    uint8_t shown = 0;

    if(snap.total == 0){
        usart0_send_string_P(PSTR("I2C FOUND none"));
        usart0_send_newline();
        list_pending = false;
        return;
    }

    usart0_send_string_P(PSTR("I2C FOUND"));

    while(print_address <= CONFIG_I2C_LAST && shown < CONFIG_I2C_LIST_PER_LINE){
        if(snap.map[print_address >> 3] & (1 << (print_address & 7))){
            usart0_send_string_P(PSTR(" "));
            scanner_send_hex(print_address);
            shown++;
        }
        print_address++;
    }
    usart0_send_newline();

    while(print_address <= CONFIG_I2C_LAST && !(snap.map[print_address >> 3] & (1 << (print_address & 7)))){
        print_address++;
    }

    if(print_address > CONFIG_I2C_LAST){
        list_pending = false;
    }
}
