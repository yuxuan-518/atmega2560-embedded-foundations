#include "loopback.h"
#include "config.h"
#include "usart.h"

#include <avr/pgmspace.h>

#define LOOP_TOTAL ((uint16_t)CONFIG_SPI_LOOP_BYTES * LOOP_PATTERNS)

typedef struct {
    uint8_t pattern;
    uint16_t index;
    uint16_t lfsr;
} loop_gen_t;

static spi_settings_t settings;
static loop_state_t state = LOOP_IDLE;
static bool in_flight = false;
static bool stop_request = false;
static loop_gen_t tx_gen;
static loop_gen_t expect_gen;
static uint16_t seed = CONFIG_SPI_LOOP_SEED;
static uint16_t sent = 0;
static uint16_t verified = 0;
static uint8_t chunk_length = 0;
static uint8_t bad = 0;
static uint8_t bad_pattern = 0;
static uint16_t bad_index = 0;
static uint8_t bad_expected = 0;
static uint8_t bad_got = 0;
static spi_result_t error_result = SPI_NONE;
static uint32_t runs = 0;
static uint32_t passes = 0;
static uint32_t fails = 0;

typedef struct {
    loop_state_t state;
    bool stopping;
    spi_settings_t settings;
    uint8_t pattern;
    uint16_t sent;
    uint16_t verified;
    uint8_t bad;
    uint8_t bad_pattern;
    uint16_t bad_index;
    uint8_t bad_expected;
    uint8_t bad_got;
    spi_result_t error;
    uint32_t runs;
    uint32_t passes;
    uint32_t fails;
    bool bus_active;
    bool bus_pending;
    uint8_t owner;
    uint8_t done;
    uint8_t length;
    bool cs_low;
    bool cancel;
    bool enabled;
    spi_settings_t applied;
    uint32_t ok;
    uint32_t timeouts;
    uint32_t lost;
    uint32_t cancelled;
    uint32_t rejects;
    spi_reject_t reject;
} loop_report_t;

static loop_report_t snap;
static uint8_t report_part = 0;

static bool event_pending = false;
static loop_state_t event_state = LOOP_IDLE;
static spi_settings_t event_settings;
static uint16_t event_verified = 0;
static uint8_t event_bad = 0;
static uint8_t event_pattern = 0;
static uint16_t event_index = 0;
static uint8_t event_expected = 0;
static uint8_t event_got = 0;
static spi_result_t event_error = SPI_NONE;

uint8_t loopback_byte(uint8_t pattern, uint16_t index, uint16_t *lfsr)
{
    switch(pattern){
        case LOOP_P00:
            return 0x00;
        case LOOP_PFF:
            return 0xFF;
        case LOOP_P55:
            return 0x55;
        case LOOP_PAA:
            return 0xAA;
        case LOOP_INC:
            return (uint8_t)index;
        default:
            *lfsr = (*lfsr >> 1) ^ ((*lfsr & 1) ? 0xB400U : 0);
            return (uint8_t)*lfsr;
    }
}

static void loopback_gen_reset(loop_gen_t *gen)
{
    gen->pattern = LOOP_P00;
    gen->index = 0;
    gen->lfsr = seed;
}

static uint8_t loopback_gen_next(loop_gen_t *gen)
{
    uint8_t value = loopback_byte(gen->pattern, gen->index, &gen->lfsr);

    gen->index++;

    if(gen->index >= CONFIG_SPI_LOOP_BYTES){
        gen->index = 0;
        gen->pattern++;
    }
    return value;
}

static void loopback_finish(loop_state_t new_state)
{
    state = new_state;
    stop_request = false;

    if(new_state == LOOP_PASS){
        passes++;
    } else if(new_state == LOOP_FAIL || new_state == LOOP_ERROR){
        fails++;
    }

    event_state = new_state;
    event_settings = settings;
    event_verified = verified;
    event_bad = bad;
    event_pattern = bad_pattern;
    event_index = bad_index;
    event_expected = bad_expected;
    event_got = bad_got;
    event_error = error_result;
    event_pending = true;
}

static void loopback_check(void)
{
    uint8_t rx[CONFIG_SPI_LOOP_CHUNK];
    uint8_t got = spi_get_data(CONFIG_SPI_OWNER_LOOP, rx, sizeof(rx));

    if(got != chunk_length){
        bad = chunk_length;
        bad_pattern = expect_gen.pattern;
        bad_index = expect_gen.index;
        bad_expected = 0;
        bad_got = got;
        loopback_finish(LOOP_FAIL);
        return;
    }

    for(uint8_t i = 0; i < got; i++){
        uint8_t pattern = expect_gen.pattern;
        uint16_t index = expect_gen.index;
        uint8_t expected = loopback_gen_next(&expect_gen);

        if(rx[i] != expected){
            if(bad == 0){
                bad_pattern = pattern;
                bad_index = index;
                bad_expected = expected;
                bad_got = rx[i];
            }
            bad++;
        }
    }

    verified += got;

    if(bad != 0){
        loopback_finish(LOOP_FAIL);
    } else if(verified >= LOOP_TOTAL){
        loopback_finish(LOOP_PASS);
    }
}

void loopback_init(void)
{
    spi_make_settings(&settings, CONFIG_SPI_MODE, CONFIG_SPI_HZ, false);
    state = LOOP_IDLE;
    in_flight = false;
    stop_request = false;
    runs = 0;
    passes = 0;
    fails = 0;
    report_part = 0;
    event_pending = false;
}

void loopback_update(uint32_t now)
{
    if(in_flight){
        spi_result_t result = spi_update(CONFIG_SPI_OWNER_LOOP, now);

        if(result == SPI_BUSY || result == SPI_NONE){
            return;
        }

        in_flight = false;

        if(result == SPI_OK){
            loopback_check();
        } else if(result == SPI_CANCELLED){
            loopback_finish(LOOP_STOPPED);
        } else {
            error_result = result;
            loopback_finish(LOOP_ERROR);
        }

        if(stop_request && state == LOOP_RUNNING){
            loopback_finish(LOOP_STOPPED);
        }
        return;
    }

    if(state != LOOP_RUNNING || spi_is_busy()){
        return;
    }

    uint8_t chunk[CONFIG_SPI_LOOP_CHUNK];
    uint8_t length = CONFIG_SPI_LOOP_CHUNK;

    if(LOOP_TOTAL - sent < length){
        length = (uint8_t)(LOOP_TOTAL - sent);
    }

    loop_gen_t next = tx_gen;

    for(uint8_t i = 0; i < length; i++){
        chunk[i] = loopback_gen_next(&next);
    }

    if(spi_transfer(CONFIG_SPI_OWNER_LOOP, &settings, false, chunk, length, now)){
        tx_gen = next;
        sent += length;
        chunk_length = length;
        in_flight = true;
    }
}

loop_start_t loopback_start(uint32_t now)
{
    (void)now;

    if(state == LOOP_RUNNING){
        return stop_request ? LOOP_START_STOPPING : LOOP_START_RUNNING;
    }

    runs++;
    seed = (uint16_t)(CONFIG_SPI_LOOP_SEED ^ (uint16_t)(runs * 0x3B1DU));

    if(seed == 0){
        seed = 1;
    }

    loopback_gen_reset(&tx_gen);
    loopback_gen_reset(&expect_gen);
    sent = 0;
    verified = 0;
    bad = 0;
    error_result = SPI_NONE;
    stop_request = false;
    state = LOOP_RUNNING;
    return LOOP_START_OK;
}

bool loopback_stopping(void)
{
    return stop_request;
}

bool loopback_stop(void)
{
    if(state != LOOP_RUNNING || stop_request){
        return false;
    }

    if(in_flight){
        stop_request = true;
        spi_cancel(CONFIG_SPI_OWNER_LOOP);
    } else {
        loopback_finish(LOOP_STOPPED);
    }
    return true;
}

bool loopback_is_running(void)
{
    return state == LOOP_RUNNING;
}

static loop_cfg_t loopback_configure(uint8_t mode, uint32_t Hz)
{
    spi_settings_t next;

    if(!spi_make_settings(&next, mode, Hz, false)){
        return LOOP_CFG_BAD;
    }

    if(state == LOOP_RUNNING || in_flight || spi_is_busy() || spi_cs_is_low()){
        return LOOP_CFG_BUSY;
    }

    settings = next;
    return LOOP_CFG_OK;
}

loop_cfg_t loopback_set_mode(uint8_t mode)
{
    return loopback_configure(mode, spi_settings_hz(&settings));
}

loop_cfg_t loopback_set_clock(uint32_t Hz)
{
    return loopback_configure(settings.mode, Hz);
}

void loopback_get_settings(spi_settings_t *out)
{
    *out = settings;
}

loop_state_t loopback_get_state(void)
{
    return state;
}

uint16_t loopback_get_sent(void)
{
    return sent;
}

uint16_t loopback_get_verified(void)
{
    return verified;
}

uint16_t loopback_get_total(void)
{
    return LOOP_TOTAL;
}

uint32_t loopback_get_runs(void)
{
    return runs;
}

uint32_t loopback_get_passes(void)
{
    return passes;
}

uint8_t loopback_get_bad(void)
{
    return bad;
}

void loopback_request_report(void)
{
    snap.state = state;
    snap.stopping = stop_request;
    snap.settings = settings;
    snap.pattern = expect_gen.pattern;
    snap.sent = sent;
    snap.verified = verified;
    snap.bad = bad;
    snap.bad_pattern = bad_pattern;
    snap.bad_index = bad_index;
    snap.bad_expected = bad_expected;
    snap.bad_got = bad_got;
    snap.error = error_result;
    snap.runs = runs;
    snap.passes = passes;
    snap.fails = fails;
    snap.bus_active = spi_is_active();
    snap.bus_pending = spi_is_busy();
    snap.owner = spi_get_owner();
    snap.done = spi_get_done();
    snap.length = spi_get_length();
    snap.cs_low = spi_cs_is_low();
    snap.cancel = spi_cancel_pending();
    snap.enabled = spi_is_enabled();
    spi_get_applied(&snap.applied);
    snap.ok = spi_get_count(SPI_OK);
    snap.timeouts = spi_get_count(SPI_TIMEOUT);
    snap.lost = spi_get_count(SPI_MODE_LOST);
    snap.cancelled = spi_get_count(SPI_CANCELLED);
    snap.rejects = spi_get_rejects();
    snap.reject = spi_get_reject();
    report_part = 1;
}

bool loopback_report_pending(void)
{
    return report_part != 0;
}

static void loopback_send_hex(uint8_t value)
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

static void loopback_send_pattern(uint8_t pattern)
{
    static const char names[][7] PROGMEM = {
        "00", "FF", "55", "AA", "INC", "RANDOM", "-"
    };

    if(pattern > LOOP_PATTERNS){
        pattern = LOOP_PATTERNS;
    }
    usart0_send_string_P(names[pattern]);
}

static void loopback_send_result(spi_result_t result)
{
    static const char names[][10] PROGMEM = {
        "NONE", "BUSY", "OK", "TIMEOUT", "MODE_LOST", "CANCELLED"
    };

    if(result <= SPI_CANCELLED){
        usart0_send_string_P(names[result]);
    }
}

static void loopback_send_settings(const spi_settings_t *value)
{
    usart0_send_string_P(PSTR("mode "));
    usart0_send_uint32(value->mode);
    usart0_send_string_P(PSTR(" (CPOL "));
    usart0_send_uint32(value->mode >> 1);
    usart0_send_string_P(PSTR(" CPHA "));
    usart0_send_uint32(value->mode & 1);
    usart0_send_string_P(PSTR("), "));
    usart0_send_uint32(spi_settings_hz(value));
    usart0_send_string_P(PSTR(" Hz"));
}

static void loopback_send_bad(uint8_t count, uint8_t pattern, uint16_t index, uint8_t expected, uint8_t got)
{
    usart0_send_string_P(PSTR("FAIL pattern "));
    loopback_send_pattern(pattern);
    usart0_send_string_P(PSTR(" byte "));
    usart0_send_uint32(index);
    usart0_send_string_P(PSTR(" expected "));
    loopback_send_hex(expected);
    usart0_send_string_P(PSTR(" got "));
    loopback_send_hex(got);
    usart0_send_string_P(PSTR(" ("));
    usart0_send_uint32(count);
    usart0_send_string_P(PSTR(" bad in chunk)"));
}

static void loopback_line_bus(void)
{
    usart0_send_string_P(PSTR("SPI BUS "));

    if(snap.bus_active){
        if(snap.owner == CONFIG_SPI_OWNER_LOOP){
            usart0_send_string_P(PSTR("LOOPBACK "));
        } else if(snap.owner == CONFIG_SPI_OWNER_DAC){
            usart0_send_string_P(PSTR("DAC "));
        } else {
            usart0_send_string_P(PSTR("owner "));
            usart0_send_uint32(snap.owner);
            usart0_send_string_P(PSTR(" "));
        }
        usart0_send_uint32(snap.done);
        usart0_send_string_P(PSTR("/"));
        usart0_send_uint32(snap.length);
        usart0_send_string_P(PSTR(" bytes"));

        if(snap.cancel){
            usart0_send_string_P(PSTR(" (cancel pending)"));
        }
    } else if(snap.bus_pending){
        usart0_send_string_P(PSTR("DONE, result not yet taken"));
    } else {
        usart0_send_string_P(PSTR("IDLE"));
    }

    if(snap.cs_low){
        usart0_send_string_P(PSTR(", CS D53 low"));
    } else {
        usart0_send_string_P(PSTR(", CS D53 high"));
    }
    usart0_send_newline();
}

static void loopback_line_hw(void)
{
    usart0_send_string_P(PSTR("SPI HW "));

    if(snap.enabled){
        loopback_send_settings(&snap.applied);
        usart0_send_string_P(PSTR(", MSB first"));
    } else {
        usart0_send_string_P(PSTR("disabled after error, re-enabled by next transfer"));
    }
    usart0_send_newline();
}

static void loopback_line_loop(void)
{
    usart0_send_string_P(PSTR("SPI LOOP "));
    loopback_send_settings(&snap.settings);
    usart0_send_string_P(PSTR(": "));

    if(snap.state == LOOP_RUNNING){
        usart0_send_string_P(PSTR("RUNNING "));

        if(snap.stopping){
            usart0_send_string_P(PSTR("(stopping) "));
        }
        loopback_send_pattern(snap.pattern);
        usart0_send_string_P(PSTR(" ("));
        usart0_send_uint32(snap.pattern < LOOP_PATTERNS ? snap.pattern + 1 : LOOP_PATTERNS);
        usart0_send_string_P(PSTR("/6), "));
    } else if(snap.state == LOOP_PASS){
        usart0_send_string_P(PSTR("PASS, "));
    } else if(snap.state == LOOP_FAIL){
        loopback_send_bad(snap.bad, snap.bad_pattern, snap.bad_index, snap.bad_expected, snap.bad_got);
        usart0_send_string_P(PSTR(", "));
    } else if(snap.state == LOOP_ERROR){
        usart0_send_string_P(PSTR("ERROR "));
        loopback_send_result(snap.error);
        usart0_send_string_P(PSTR(", "));
    } else if(snap.state == LOOP_STOPPED){
        usart0_send_string_P(PSTR("STOPPED, "));
    } else {
        usart0_send_string_P(PSTR("IDLE, "));
    }

    usart0_send_uint32(snap.verified);
    usart0_send_string_P(PSTR("/"));
    usart0_send_uint32(LOOP_TOTAL);
    usart0_send_string_P(PSTR(" verified, "));
    usart0_send_uint32(snap.sent);
    usart0_send_string_P(PSTR(" sent"));
    usart0_send_newline();
}

static void loopback_line_runs(void)
{

    usart0_send_string_P(PSTR("SPI LOOP RUNS "));
    usart0_send_uint32(snap.runs);
    usart0_send_string_P(PSTR(", PASS "));
    usart0_send_uint32(snap.passes);
    usart0_send_string_P(PSTR(", FAIL "));
    usart0_send_uint32(snap.fails);
    usart0_send_string_P(PSTR(" (MOSI D51 -> MISO D50, MCU check only)"));
    usart0_send_newline();
}

static void loopback_line_stats(void)
{
    usart0_send_string_P(PSTR("SPI STATS ok "));
    usart0_send_uint32(snap.ok);
    usart0_send_string_P(PSTR(", timeout "));
    usart0_send_uint32(snap.timeouts);
    usart0_send_string_P(PSTR(", mode lost "));
    usart0_send_uint32(snap.lost);
    usart0_send_string_P(PSTR(", cancelled "));
    usart0_send_uint32(snap.cancelled);
    usart0_send_string_P(PSTR(", rejected "));
    usart0_send_uint32(snap.rejects);

    if(snap.reject == SPI_REJECT_BUSY){
        usart0_send_string_P(PSTR(" (last BUSY)"));
    } else if(snap.reject == SPI_REJECT_LENGTH){
        usart0_send_string_P(PSTR(" (last LENGTH)"));
    } else if(snap.reject == SPI_REJECT_PARAM){
        usart0_send_string_P(PSTR(" (last PARAM)"));
    }
    usart0_send_newline();
}

void loopback_send_report(void)
{
    if(report_part == 1){
        loopback_line_bus();
    } else if(report_part == 2){
        loopback_line_hw();
    } else if(report_part == 3){
        loopback_line_loop();
    } else if(report_part == 4){
        loopback_line_runs();
    } else {
        loopback_line_stats();
        report_part = 0;
        return;
    }
    report_part++;
}

bool loopback_event_pending(void)
{
    return event_pending;
}

void loopback_send_event(void)
{
    event_pending = false;

    usart0_send_string_P(PSTR("SPI LOOPBACK "));

    if(event_state == LOOP_PASS){
        usart0_send_string_P(PSTR("PASS "));
    } else if(event_state == LOOP_FAIL){
        loopback_send_bad(event_bad, event_pattern, event_index, event_expected, event_got);
        usart0_send_string_P(PSTR(", "));
    } else if(event_state == LOOP_ERROR){
        usart0_send_string_P(PSTR("ERROR "));
        loopback_send_result(event_error);
        usart0_send_string_P(PSTR(", CS released, "));
    } else {
        usart0_send_string_P(PSTR("STOPPED, "));
    }

    usart0_send_uint32(event_verified);
    usart0_send_string_P(PSTR("/"));
    usart0_send_uint32(LOOP_TOTAL);
    usart0_send_string_P(PSTR(" bytes, "));
    loopback_send_settings(&event_settings);
    usart0_send_newline();
}
