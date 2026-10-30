#include "dac.h"
#include "config.h"
#include "mcp4921.h"
#include "usart.h"

#include <avr/io.h>
#include <avr/pgmspace.h>
#include <util/atomic.h>

static bool ready = false;
static spi_settings_t settings;
static dac_state_t state = DAC_IDLE;
static bool pending = false;
static uint16_t pending_value = 0;
static uint32_t pending_ms = 0;
static uint16_t sending_value = 0;
static bool have_value = false;
static uint16_t value = 0;
static uint32_t sent = 0;
static uint32_t errors = 0;
static dac_error_t error = DAC_ERROR_NONE;
static spi_result_t last_result = SPI_NONE;

typedef struct {
    dac_state_t state;
    bool pending;
    uint16_t pending_value;
    uint16_t sending_value;
    uint8_t done;
    bool have_value;
    uint16_t value;
    uint32_t sent;
    uint32_t errors;
    dac_error_t error;
    spi_result_t result;
} dac_report_t;

static dac_report_t snap;
static uint8_t report_part = 0;

static bool event_pending = false;
static bool event_ok = false;
static uint16_t event_value = 0;
static dac_error_t event_error = DAC_ERROR_NONE;
static spi_result_t event_result = SPI_NONE;

static void dac_ldac(bool high)
{
#if CONFIG_DAC_LDAC_PIN
    ATOMIC_BLOCK(ATOMIC_RESTORESTATE){
        if(high){
            PORTL |= (1 << PORTL0);
        } else {
            PORTL &= ~(1 << PORTL0);
        }
    }
#else
    (void)high;
#endif
}

static void dac_event(bool ok, uint16_t event_val)
{
    event_ok = ok;
    event_value = event_val;
    event_error = error;
    event_result = last_result;
    event_pending = true;
}

static void dac_fail(dac_error_t new_error, spi_result_t result, uint16_t failed_value)
{
    error = new_error;
    last_result = result;
    errors++;
    state = DAC_IDLE;
    dac_event(false, failed_value);
}

static void dac_complete(void)
{
    value = sending_value;
    have_value = true;
    sent++;
    error = DAC_ERROR_NONE;
    last_result = SPI_OK;
    state = DAC_IDLE;
    dac_event(true, value);
}

bool dac_init(void)
{
    state = DAC_IDLE;
    pending = false;
    have_value = false;
    sent = 0;
    errors = 0;
    error = DAC_ERROR_NONE;
    last_result = SPI_NONE;
    report_part = 0;
    event_pending = false;

#if CONFIG_DAC_LDAC_PIN
    ATOMIC_BLOCK(ATOMIC_RESTORESTATE){
        PORTL |= (1 << PORTL0);
        DDRL |= (1 << DDL0);
    }
#endif

    ready = mcp4921_mode_ok(CONFIG_DAC_SPI_MODE) && spi_make_settings(&settings, CONFIG_DAC_SPI_MODE, CONFIG_DAC_SPI_HZ, false);
    return ready;
}

void dac_update(uint32_t now)
{
    if(state == DAC_SENDING){
        spi_result_t result = spi_update(CONFIG_SPI_OWNER_DAC, now);

        if(result == SPI_BUSY || result == SPI_NONE){
            return;
        }

        if(result != SPI_OK){
            dac_fail(DAC_ERROR_SPI, result, sending_value);
            return;
        }

#if CONFIG_DAC_LDAC_PIN
        dac_ldac(false);
        state = DAC_LDAC;
#else
        dac_complete();
#endif
        return;
    }

    if(state == DAC_LDAC){
        dac_ldac(true);
        dac_complete();
        return;
    }

    if(!pending){
        return;
    }

    if(spi_is_busy()){
        if(now - pending_ms > CONFIG_DAC_BUS_WAIT_MS){
            pending = false;
            dac_fail(DAC_ERROR_BUS_BUSY, SPI_BUSY, pending_value);
        }
        return;
    }

    uint8_t bytes[MCP4921_BYTES];

    if(!mcp4921_command(pending_value, CONFIG_DAC_BUFFERED, CONFIG_DAC_GAIN_1X, true, bytes)){
        pending = false;
        return;
    }

    if(spi_transfer(CONFIG_SPI_OWNER_DAC, &settings, true, bytes, MCP4921_BYTES, now)){
        sending_value = pending_value;
        pending = false;
        state = DAC_SENDING;
    }
}

bool dac_set(uint16_t new_value, uint32_t now)
{
    if(!ready || new_value > MCP4921_MAX){
        return false;
    }

    pending_value = new_value;
    pending_ms = now;
    pending = true;
    return true;
}

bool dac_is_ready(void)
{
    return ready;
}

dac_state_t dac_get_state(void)
{
    return state;
}

bool dac_has_pending(void)
{
    return pending;
}

bool dac_has_value(void)
{
    return have_value;
}

uint16_t dac_get_value(void)
{
    return value;
}

uint32_t dac_get_sent(void)
{
    return sent;
}

uint32_t dac_get_errors(void)
{
    return errors;
}

dac_error_t dac_get_error(void)
{
    return error;
}

spi_result_t dac_get_result(void)
{
    return last_result;
}

void dac_request_report(void)
{
    snap.state = state;
    snap.pending = pending;
    snap.pending_value = pending_value;
    snap.sending_value = sending_value;
    snap.done = 0;

    if(state == DAC_SENDING && spi_get_owner() == CONFIG_SPI_OWNER_DAC){
        snap.done = spi_get_done();
    }

    snap.have_value = have_value;
    snap.value = value;
    snap.sent = sent;
    snap.errors = errors;
    snap.error = error;
    snap.result = last_result;
    report_part = 1;
}

bool dac_report_pending(void)
{
    return report_part != 0;
}

static void dac_send_volts(uint16_t code)
{
    uint32_t uv = mcp4921_microvolts(code, CONFIG_DAC_VREF_MV, CONFIG_DAC_GAIN_1X);
    uint32_t frac = uv % 1000;

    usart0_send_uint32(uv / 1000);
    usart0_send_string_P(PSTR("."));

    if(frac < 100){
        usart0_send_string_P(PSTR("0"));
    }
    if(frac < 10){
        usart0_send_string_P(PSTR("0"));
    }
    usart0_send_uint32(frac);
    usart0_send_string_P(PSTR(" mV"));
}

static void dac_send_result(spi_result_t result)
{
    static const char names[][10] PROGMEM = {
        "NONE", "BUS_BUSY", "OK", "TIMEOUT", "MODE_LOST", "CANCELLED"
    };

    if(result <= SPI_CANCELLED){
        usart0_send_string_P(names[result]);
    }
}

static void dac_line_state(void)
{
    usart0_send_string_P(PSTR("DAC MCP4921 (CS D53"));
#if CONFIG_DAC_LDAC_PIN
    usart0_send_string_P(PSTR(", LDAC D49 pulse"));
#else
    usart0_send_string_P(PSTR(", LDAC tied low"));
#endif
    usart0_send_string_P(PSTR("): "));

    if(!ready){
        usart0_send_string_P(PSTR("DISABLED, DAC SPI config invalid (MCP4921 needs mode 0 or 3)"));
        usart0_send_newline();
        report_part = 0;
        return;
    }

    if(snap.state == DAC_SENDING){
        usart0_send_string_P(PSTR("SENDING "));
        usart0_send_uint32(snap.sending_value);
        usart0_send_string_P(PSTR(" ("));
        usart0_send_uint32(snap.done);
        usart0_send_string_P(PSTR("/2 bytes)"));
    } else if(snap.state == DAC_LDAC){
        usart0_send_string_P(PSTR("LDAC pulse"));
    } else if(snap.pending){
        usart0_send_string_P(PSTR("WAIT_BUS"));
    } else {
        usart0_send_string_P(PSTR("IDLE"));
    }

    if(snap.pending){
        usart0_send_string_P(PSTR(", queued "));
        usart0_send_uint32(snap.pending_value);
    }
    usart0_send_newline();
}

static void dac_line_value(void)
{
    usart0_send_string_P(PSTR("DAC LAST SENT "));

    if(snap.have_value){
        usart0_send_uint32(snap.value);
        usart0_send_string_P(PSTR(" -> "));
        dac_send_volts(snap.value);
        usart0_send_string_P(PSTR(" theory (VREF "));
        usart0_send_uint32(CONFIG_DAC_VREF_MV);
        usart0_send_string_P(PSTR(" mV x "));
        usart0_send_uint32(snap.value);
        usart0_send_string_P(PSTR(" / 4096, gain "));
        if(CONFIG_DAC_GAIN_1X){
            usart0_send_string_P(PSTR("1x)"));
        } else {
            usart0_send_string_P(PSTR("2x)"));
        }
    } else {
        usart0_send_string_P(PSTR("none"));
    }
    usart0_send_newline();
}

static void dac_line_count(void)
{
    usart0_send_string_P(PSTR("DAC WRITES "));
    usart0_send_uint32(snap.sent);
    usart0_send_string_P(PSTR(", ERRORS "));
    usart0_send_uint32(snap.errors);
    usart0_send_string_P(PSTR(", LAST "));

    if(snap.error == DAC_ERROR_NONE){
        usart0_send_string_P(PSTR("none"));
    } else if(snap.error == DAC_ERROR_BUS_BUSY){
        usart0_send_string_P(PSTR("BUS_BUSY (waited 50 ms)"));
    } else {
        usart0_send_string_P(PSTR("SPI "));
        dac_send_result(snap.result);
    }
    usart0_send_newline();
}

static void dac_line_config(void)
{
    usart0_send_string_P(PSTR("DAC SPI mode "));
    usart0_send_uint32(settings.mode);
    usart0_send_string_P(PSTR(", "));
    usart0_send_uint32(spi_settings_hz(&settings));
    usart0_send_string_P(PSTR(" Hz fixed, word 0/BUF "));
    usart0_send_uint32(CONFIG_DAC_BUFFERED);
    usart0_send_string_P(PSTR("/GA "));
    usart0_send_uint32(CONFIG_DAC_GAIN_1X);
    usart0_send_string_P(PSTR("/SHDN 1/D11-D0, no ACK"));
    usart0_send_newline();
}

void dac_send_report(void)
{
    if(report_part == 1){
        dac_line_state();
        if(report_part == 0){
            return;
        }
    } else if(report_part == 2){
        dac_line_value();
    } else if(report_part == 3){
        dac_line_count();
    } else {
        dac_line_config();
        report_part = 0;
        return;
    }
    report_part++;
}

bool dac_event_pending(void)
{
    return event_pending;
}

void dac_send_event(void)
{
    event_pending = false;

    usart0_send_string_P(PSTR("DAC "));
    usart0_send_uint32(event_value);

    if(event_ok){
        usart0_send_string_P(PSTR(" sent, "));
        dac_send_volts(event_value);
        usart0_send_string_P(PSTR(" theory"));
    } else {
        usart0_send_string_P(PSTR(" FAILED "));

        if(event_error == DAC_ERROR_BUS_BUSY){
            usart0_send_string_P(PSTR("BUS_BUSY"));
        } else {
            dac_send_result(event_result);
        }

        usart0_send_string_P(PSTR(", output keeps "));

        if(have_value){
            usart0_send_uint32(value);
        } else {
            usart0_send_string_P(PSTR("power-on value"));
        }
    }
    usart0_send_newline();
}
