#include "nh3.h"
#include "nh3_protocol.h"
#include "config.h"
#include "frame.h"
#include "usart1.h"
#include "usart.h"
#include "report.h"
#include <avr/pgmspace.h>

static frame_parser_t parser;
static const frame_format_t *format = 0;

static nh3_state_t state = NH3_NO_DATA;
static uint32_t baud = CONFIG_NH3_DEFAULT_BAUD;

static bool link_seen = false;
static bool sensor_warmup = false;
static uint32_t last_frame_ms = 0;
static uint32_t warm_start_ms = 0;

static bool reading_seen = false;
static uint32_t last_ppb = 0;
static uint32_t last_valid_ms = 0;

static uint32_t bytes_rx = 0;
static uint32_t bad_field = 0;
static uint32_t gaps = 0;
static uint32_t overflow = 0;
static uint32_t hw_errors = 0;
static uint32_t trace_dropped = 0;

static bool raw_enabled = false;
static uint8_t raw_line[CONFIG_NH3_RAW_LINE_BYTES];
static uint8_t raw_count = 0;
static uint32_t raw_last_ms = 0;

static void nh3_send_hex(uint8_t value)
{
    const char hex[] = "0123456789ABCDEF";
    usart0_tx_send(hex[value >> 4]);
    usart0_tx_send(hex[value & 0x0F]);
}

static void nh3_send_ppb(uint32_t ppb)
{
    uint32_t frac = ppb % 1000;

    usart0_send_uint32(ppb / 1000);
    usart0_send_string_P(PSTR("."));
    usart0_tx_send('0' + frac / 100);
    usart0_tx_send('0' + (frac / 10) % 10);
    usart0_tx_send('0' + frac % 10);
}

static void nh3_trace(const char tag[], const uint8_t data[], uint8_t length)
{
    if(!raw_enabled){
        return;
    }

    if(usart0_tx_free() < (uint16_t)length * 3 + 16 + REPORT_TX_RESERVE){
        trace_dropped++;
        return;
    }

    usart0_send_string_P(PSTR("RAW "));
    usart0_send_string(tag);

    for(uint8_t i = 0; i < length; i++){
        usart0_tx_send(' ');
        nh3_send_hex(data[i]);
    }

    usart0_send_newline();
}

static void nh3_raw_flush(void)
{
    if(raw_count > 0){
        nh3_trace("BYTES", raw_line, raw_count);
        raw_count = 0;
    }
}

static void nh3_raw_byte(uint8_t data, uint32_t now)
{
    raw_line[raw_count] = data;
    raw_count++;
    raw_last_ms = now;

    if(raw_count >= CONFIG_NH3_RAW_LINE_BYTES){
        nh3_raw_flush();
    }
}

static void nh3_frame_ok(uint32_t now)
{
    uint32_t ppb = 0;
    nh3_decode_t result = nh3_protocol_decode(parser.frame, &ppb);

    if(result == NH3_DECODE_BAD_FIELD || result == NH3_DECODE_RANGE){
        bad_field++;
        nh3_trace("FIELD", parser.frame, format->length);
        return;
    }

    nh3_trace("OK", parser.frame, format->length);

    if(state == NH3_STALE && now - last_frame_ms >= CONFIG_NH3_REWARM_MS){
        warm_start_ms = now;
    }

    link_seen = true;
    last_frame_ms = now;
    sensor_warmup = (result == NH3_DECODE_WARMUP);

    if(!sensor_warmup && now - warm_start_ms >= CONFIG_NH3_WARMUP_MS){
        reading_seen = true;
        last_ppb = ppb;
        last_valid_ms = now;
    }
}

static void nh3_update_state(uint32_t now)
{
    if(format == 0){
        state = NH3_DISABLED;
    } else if(!link_seen){
        state = NH3_NO_DATA;
    } else if(now - last_frame_ms > CONFIG_NH3_TIMEOUT_MS){
        state = NH3_STALE;
    } else if(sensor_warmup || now - warm_start_ms < CONFIG_NH3_WARMUP_MS){
        state = NH3_WARMUP;
    } else if(reading_seen && last_valid_ms == last_frame_ms){
        state = NH3_VALID;
    } else {
        state = NH3_WARMUP;
    }
}

bool nh3_send_active_command(void)
{
    const uint8_t *command = 0;
    uint8_t length = nh3_protocol_active_command(&command);

    if(length == 0){
        return false;
    }

    for(uint8_t i = 0; i < length; i++){
        usart1_tx_send(command[i]);
    }

    return true;
}

void nh3_init(uint32_t now)
{
    format = nh3_protocol_format();

    if(!frame_format_valid(format)){
        format = 0;
    }

    if(format != 0){
        frame_init(&parser, format);
    }

    warm_start_ms = now;
    usart1_init(baud);
    nh3_send_active_command();
    nh3_update_state(now);
}

void nh3_update(uint32_t now)
{
    uint8_t data = 0;

    overflow += usart1_take_overflow();
    hw_errors += usart1_take_errors();

    for(uint8_t i = 0; i < CONFIG_NH3_PARSE_BUDGET; i++){
        usart1_rx_t rx = usart1_rx_get(&data);

        if(rx == USART1_RX_NONE){
            break;
        }

        if(rx == USART1_RX_GAP){
            gaps++;

            if(format != 0){
                frame_drop_partial(&parser);
            } else {
                nh3_raw_flush();
            }
            continue;
        }

        bytes_rx++;

        if(format == 0){
            nh3_raw_byte(data, now);
            continue;
        }

        frame_result_t result = frame_push(&parser, data, now);

        if(result == FRAME_OK){
            nh3_frame_ok(now);
        } else if(result == FRAME_BAD_CHECK){
            nh3_trace("BAD", parser.frame, format->length);
        }
    }

    if(format != 0){
        frame_check_gap(&parser, now);
    } else if(raw_count > 0 && now - raw_last_ms >= CONFIG_NH3_RAW_GAP_MS){
        nh3_raw_flush();
    }

    nh3_update_state(now);
}

bool nh3_set_baud(uint32_t new_baud)
{
    if(!usart1_init(new_baud)){
        return false;
    }

    baud = new_baud;

    if(format != 0){
        frame_drop_partial(&parser);
    }
    raw_count = 0;
    return true;
}

void nh3_set_raw(bool enable)
{
    raw_enabled = enable;
    raw_count = 0;
}

void nh3_reset_counters(void)
{
    bytes_rx = 0;
    bad_field = 0;
    gaps = 0;
    overflow = 0;
    hw_errors = 0;
    trace_dropped = 0;

    if(format != 0){
        parser.ok = 0;
        parser.bad_check = 0;
        parser.incomplete = 0;
        parser.skipped = 0;
    }
}

nh3_state_t nh3_get_state(void)
{
    return state;
}

bool nh3_get_ppb(uint32_t *ppb)
{
    if(state != NH3_VALID){
        return false;
    }

    *ppb = last_ppb;
    return true;
}

void nh3_send_state(nh3_state_t s)
{
    switch(s){
        case NH3_DISABLED:
            usart0_send_string_P(PSTR("NO_PROTOCOL"));
            break;
        case NH3_NO_DATA:
            usart0_send_string_P(PSTR("NO_DATA"));
            break;
        case NH3_WARMUP:
            usart0_send_string_P(PSTR("WARMUP"));
            break;
        case NH3_VALID:
            usart0_send_string_P(PSTR("VALID"));
            break;
        case NH3_STALE:
            usart0_send_string_P(PSTR("STALE"));
            break;
    }
}

void nh3_send_brief(uint32_t now)
{
    usart0_send_string_P(PSTR("NH3: "));
    nh3_send_state(state);
    usart0_send_string_P(PSTR(", PPM: "));

    if(reading_seen){
        nh3_send_ppb(last_ppb);

        if(state != NH3_VALID){
            usart0_send_string_P(PSTR(" (old)"));
        }

        usart0_send_string_P(PSTR(", AGE: "));
        usart0_send_uint32(now - last_valid_ms);
        usart0_send_string_P(PSTR(" ms"));
    } else {
        usart0_send_string_P(PSTR("-"));
    }
    usart0_send_newline();
}

void nh3_send_status(uint32_t now)
{
    nh3_send_brief(now);

    usart0_send_string_P(PSTR("NH3 RX: "));
    usart0_send_uint32(bytes_rx);
    usart0_send_string_P(PSTR(" B, OK: "));

    if(format != 0){
        usart0_send_uint32(parser.ok);
        usart0_send_string_P(PSTR(", BAD: "));
        usart0_send_uint32(parser.bad_check + bad_field);
        usart0_send_string_P(PSTR(", PART: "));
        usart0_send_uint32(parser.incomplete);
    } else {
        usart0_send_string_P(PSTR("-"));
    }

    usart0_send_string_P(PSTR(", OVF: "));
    usart0_send_uint32(overflow);
    usart0_send_string_P(PSTR(", HWERR: "));
    usart0_send_uint32(hw_errors);
    usart0_send_string_P(PSTR(", GAP: "));
    usart0_send_uint32(gaps);
    usart0_send_string_P(PSTR(", TRACE DROP: "));
    usart0_send_uint32(trace_dropped);
    usart0_send_newline();

    usart0_send_string_P(PSTR("NH3 PROTOCOL: "));
    usart0_send_string(nh3_protocol_name());
    usart0_send_string_P(PSTR(", BAUD: "));
    usart0_send_uint32(baud);
    usart0_send_string_P(PSTR(", RANGE: "));
    usart0_send_uint32(CONFIG_NH3_RANGE_PPM);
    usart0_send_string_P(PSTR(" ppm, WARMUP: "));
    usart0_send_uint32(CONFIG_NH3_WARMUP_MS / 1000);
    usart0_send_string_P(PSTR(" s, RAW: "));
    usart0_send_uint32(raw_enabled);
    usart0_send_newline();
}
