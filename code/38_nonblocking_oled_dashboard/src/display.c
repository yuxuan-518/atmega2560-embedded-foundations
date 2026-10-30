#include "display.h"
#include "display_view.h"
#include "config.h"
#include "ssd1306.h"
#include "gfx.h"
#include "usart.h"

#include <avr/pgmspace.h>

typedef enum {
    DISPLAY_EVENT_ON,
    DISPLAY_EVENT_RETRY,
    DISPLAY_EVENT_FAILED,
    DISPLAY_EVENT_OFF
} display_event_t;

static bool ready = false;
static display_state_t state = DISPLAY_OFF;
static display_state_t fail_state = DISPLAY_OFF;
static display_frame_t frame = DISPLAY_FRAME_IDLE;
static uint8_t page = 0;
static uint8_t render_row = 0;
static bool page_change = false;
static bool full_request = false;
static bool frame_full = false;
static uint32_t merged = 0;
static uint32_t state_ms = 0;
static uint32_t check_ms = 0;
static uint32_t frame_ms = 0;
static uint16_t frame_bytes = 0;
static uint16_t frame_sent = 0;
static uint8_t win_page_first = 0;
static uint8_t win_page_last = 0;
static uint8_t win_col_first = 0;
static uint8_t win_col_last = 0;
static uint8_t consecutive = 0;
static uint32_t errors = 0;
static uint32_t backoff = 0;
static uint32_t frames = 0;
static uint32_t full_frames = 0;
static uint32_t partial_frames = 0;
static uint32_t unchanged = 0;
static bool have_full = false;
static uint32_t full_ms = 0;
static uint32_t full_took = 0;
static uint32_t last_took = 0;
static uint16_t last_bytes = 0;
static i2c_result_t last_error = I2C_NONE;
static i2c_phase_t last_phase = I2C_PHASE_IDLE;

typedef struct {
    display_state_t state;
    display_state_t fail_state;
    display_frame_t frame;
    uint8_t page;
    bool page_change;
    bool full_request;
    bool frame_full;
    uint8_t render_row;
    uint16_t frame_bytes;
    uint16_t frame_sent;
    uint32_t next_check;
    uint32_t frames;
    uint32_t full_frames;
    uint32_t partial_frames;
    uint32_t unchanged;
    uint32_t merged;
    uint32_t transactions;
    bool have_full;
    uint32_t full_ms;
    uint32_t full_took;
    uint32_t last_took;
    uint16_t last_bytes;
    uint32_t errors;
    uint8_t consecutive;
    i2c_result_t error;
    i2c_phase_t phase;
    uint32_t retry_left;
} display_report_t;

static display_report_t snap;
static uint8_t report_part = 0;

static bool event_pending = false;
static display_event_t event_type = DISPLAY_EVENT_OFF;
static uint8_t event_count = 0;
static uint32_t event_backoff = 0;
static i2c_result_t event_error = I2C_NONE;
static i2c_phase_t event_phase = I2C_PHASE_IDLE;
static display_state_t event_state = DISPLAY_OFF;
static uint32_t event_took = 0;
static bool event_ok = true;

static void display_enter(display_state_t new_state, uint32_t now)
{
    state = new_state;
    state_ms = now;
}

static void display_event(display_event_t type)
{
    event_type = type;
    event_count = consecutive;
    event_backoff = backoff;
    event_error = last_error;
    event_phase = last_phase;
    event_state = fail_state;
    event_took = full_took;
    event_pending = true;
}

static void display_start_init(uint32_t now)
{
    frame = DISPLAY_FRAME_IDLE;
    gfx_mark_all();

    if(ssd1306_start_init()){
        display_enter(DISPLAY_INIT, now);
    }
}

static void display_fail(uint32_t now)
{
    errors++;
    consecutive++;
    last_error = ssd1306_get_error();
    last_phase = ssd1306_get_error_phase();
    fail_state = state;
    frame = DISPLAY_FRAME_IDLE;
    gfx_mark_all();

    if(consecutive > CONFIG_DISPLAY_RETRIES){
        backoff = 0;
        display_enter(DISPLAY_FAILED, now);
        display_event(DISPLAY_EVENT_FAILED);
        return;
    }

    backoff = CONFIG_DISPLAY_RETRY_MS << (consecutive - 1);

    if(backoff > CONFIG_DISPLAY_RETRY_MAX_MS){
        backoff = CONFIG_DISPLAY_RETRY_MAX_MS;
    }

    display_enter(DISPLAY_BACKOFF, now);
    display_event(DISPLAY_EVENT_RETRY);
}

static void display_send_window(void)
{
    gfx_next_window(&win_page_first, &win_page_last, &win_col_first, &win_col_last);
    ssd1306_start_write(gfx_frame(), win_page_first, win_page_last, win_col_first, win_col_last);
}

static void display_frame_done(uint32_t now)
{
    frames++;
    last_took = now - frame_ms;
    last_bytes = frame_bytes;
    frame = DISPLAY_FRAME_IDLE;

    if(frame_full){
        full_frames++;
        have_full = true;
        full_ms = now;
        full_took = last_took;
    } else {
        partial_frames++;
    }

    if(state == DISPLAY_FIRST){
        ssd1306_start_command(SSD1306_DISPLAY_ON);
        display_enter(DISPLAY_SHOW, now);
    }
}

static void display_frame(uint32_t now)
{
    if(frame == DISPLAY_FRAME_SEND){
        ssd1306_result_t result = ssd1306_update(now);

        if(result == SSD1306_BUSY){
            return;
        }

        if(result != SSD1306_OK){
            display_fail(now);
            return;
        }

        gfx_clean_window(win_page_first, win_page_last, win_col_first, win_col_last);
        frame_sent = (uint16_t)(frame_sent + (uint16_t)(win_col_last - win_col_first + 1) * (uint16_t)(win_page_last - win_page_first + 1));

        if(gfx_dirty_any()){
            display_send_window();
        } else {
            display_frame_done(now);
        }
        return;
    }

    if(frame == DISPLAY_FRAME_RENDER){
        display_view_render(render_row);
        render_row++;

        if(render_row < DISPLAY_VIEW_ROWS){
            return;
        }

        if(frame_full){
            gfx_mark_all();
        }

        if(!gfx_dirty_any()){
            unchanged++;
            frame = DISPLAY_FRAME_IDLE;
            return;
        }

        frame_bytes = gfx_dirty_bytes();
        frame_sent = 0;
        frame = DISPLAY_FRAME_SEND;
        display_send_window();
        return;
    }

    bool first = state == DISPLAY_FIRST;

    if(!first && !page_change && !full_request && now - check_ms < CONFIG_DISPLAY_PERIOD_MS){
        return;
    }

    check_ms = now;
    frame_ms = now;
    frame_full = first || page_change || full_request;
    page_change = false;
    full_request = false;
    display_view_take(now, page, errors);
    render_row = 0;
    frame = DISPLAY_FRAME_RENDER;
}

bool display_init(uint32_t now)
{
    gfx_init();
    ready = ssd1306_setup(CONFIG_I2C_OWNER_DISPLAY, CONFIG_DISPLAY_ADDRESS);
    frame = DISPLAY_FRAME_IDLE;
    consecutive = 0;
    errors = 0;
    event_pending = false;
    report_part = 0;

    if(ready && CONFIG_DISPLAY_AUTOSTART){
        display_enter(DISPLAY_WAIT, now);
    } else {
        display_enter(DISPLAY_OFF, now);
    }
    return ready;
}

void display_update(uint32_t now)
{
    ssd1306_result_t result;

    switch(state){
        case DISPLAY_WAIT:
            if(now - state_ms >= CONFIG_DISPLAY_POWER_MS){
                display_start_init(now);
            }
            return;

        case DISPLAY_INIT:
            result = ssd1306_update(now);

            if(result == SSD1306_OK){
                display_enter(DISPLAY_FIRST, now);
            } else if(result != SSD1306_BUSY){
                display_fail(now);
            }
            return;

        case DISPLAY_FIRST:
        case DISPLAY_RUN:
            display_frame(now);
            return;

        case DISPLAY_SHOW:
            result = ssd1306_update(now);

            if(result == SSD1306_OK){
                consecutive = 0;
                backoff = 0;
                check_ms = now;
                display_enter(DISPLAY_RUN, now);
                display_event(DISPLAY_EVENT_ON);
            } else if(result != SSD1306_BUSY){
                display_fail(now);
            }
            return;

        case DISPLAY_BACKOFF:
            if(now - state_ms >= backoff){
                display_start_init(now);
            }
            return;

        case DISPLAY_STOPPING:
            result = ssd1306_update(now);

            if(result == SSD1306_BUSY){
                return;
            }

            frame = DISPLAY_FRAME_IDLE;
            gfx_mark_all();
            ssd1306_start_command(SSD1306_DISPLAY_OFF);
            display_enter(DISPLAY_BLANK, now);
            return;

        case DISPLAY_BLANK:
            result = ssd1306_update(now);

            if(result == SSD1306_BUSY){
                return;
            }

            event_ok = result == SSD1306_OK;

            if(!event_ok){
                last_error = ssd1306_get_error();
                last_phase = ssd1306_get_error_phase();
            }
            display_enter(DISPLAY_OFF, now);
            display_event(DISPLAY_EVENT_OFF);
            return;

        default:
            return;
    }
}

display_on_t display_on(uint32_t now)
{
    if(!ready){
        return DISPLAY_ON_BAD_CONFIG;
    }

    if(state == DISPLAY_STOPPING || state == DISPLAY_BLANK){
        return DISPLAY_ON_STOPPING;
    }

    if(state != DISPLAY_OFF && state != DISPLAY_FAILED){
        return DISPLAY_ON_ALREADY;
    }

    consecutive = 0;
    backoff = 0;
    full_request = false;
    page_change = false;
    display_start_init(now);
    return DISPLAY_ON_OK;
}

bool display_off(void)
{
    if(state == DISPLAY_OFF){
        return false;
    }

    if(state == DISPLAY_STOPPING || state == DISPLAY_BLANK){
        return true;
    }

    frame = DISPLAY_FRAME_IDLE;
    full_request = false;

    if(ssd1306_is_busy()){
        ssd1306_abort();
        state = DISPLAY_STOPPING;
    } else {
        ssd1306_start_command(SSD1306_DISPLAY_OFF);
        state = DISPLAY_BLANK;
    }
    return true;
}

bool display_set_page(uint8_t new_page)
{
    if(new_page >= CONFIG_DISPLAY_PAGES){
        return false;
    }

    if(new_page != page){
        page = new_page;
        page_change = true;
    }
    return true;
}

bool display_refresh(void)
{
    if(!display_is_active()){
        return false;
    }

    if(full_request){
        merged++;
    }
    full_request = true;
    return true;
}

display_state_t display_get_state(void)
{
    return state;
}

display_frame_t display_get_frame(void)
{
    return frame;
}

bool display_is_active(void)
{
    return state != DISPLAY_OFF && state != DISPLAY_FAILED && state != DISPLAY_STOPPING && state != DISPLAY_BLANK;
}

uint8_t display_get_page(void)
{
    return page;
}

uint32_t display_get_frames(void)
{
    return frames;
}

uint32_t display_get_full_frames(void)
{
    return full_frames;
}

uint32_t display_get_partial_frames(void)
{
    return partial_frames;
}

uint32_t display_get_unchanged(void)
{
    return unchanged;
}

uint32_t display_get_errors(void)
{
    return errors;
}

uint8_t display_get_consecutive(void)
{
    return consecutive;
}

uint32_t display_get_merged(void)
{
    return merged;
}

uint32_t display_get_full_ms(void)
{
    return full_ms;
}

uint32_t display_get_full_took(void)
{
    return full_took;
}

void display_request_report(uint32_t now)
{
    snap.state = state;
    snap.fail_state = fail_state;
    snap.frame = frame;
    snap.page = page;
    snap.page_change = page_change;
    snap.full_request = full_request;
    snap.frame_full = frame_full;
    snap.render_row = render_row;
    snap.frame_bytes = frame_bytes;
    snap.frame_sent = frame_sent;
    snap.next_check = 0;

    if(now - check_ms < CONFIG_DISPLAY_PERIOD_MS){
        snap.next_check = CONFIG_DISPLAY_PERIOD_MS - (now - check_ms);
    }

    if(frame == DISPLAY_FRAME_SEND && ssd1306_get_job() == SSD1306_JOB_WRITE){
        snap.frame_sent = (uint16_t)(frame_sent + ssd1306_get_sent());
    }

    snap.frames = frames;
    snap.full_frames = full_frames;
    snap.partial_frames = partial_frames;
    snap.unchanged = unchanged;
    snap.merged = merged;
    snap.transactions = ssd1306_get_transactions();
    snap.have_full = have_full;
    snap.full_ms = full_ms;
    snap.full_took = full_took;
    snap.last_took = last_took;
    snap.last_bytes = last_bytes;
    snap.errors = errors;
    snap.consecutive = consecutive;
    snap.error = last_error;
    snap.phase = last_phase;
    snap.retry_left = 0;

    if(state == DISPLAY_BACKOFF && now - state_ms < backoff){
        snap.retry_left = backoff - (now - state_ms);
    }
    report_part = 1;
}

bool display_report_pending(void)
{
    return report_part != 0;
}

static void display_send_hex(uint8_t value)
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

static void display_send_state(display_state_t value)
{
    static const char names[][12] PROGMEM = {
        "OFF", "POWER_WAIT", "INIT", "FIRST_FRAME", "DISPLAY_ON", "RUN", "RETRY_WAIT", "FAILED", "STOPPING", "OFF_CMD"
    };

    if(value <= DISPLAY_BLANK){
        usart0_send_string_P(names[value]);
    }
}

static void display_send_error(i2c_result_t result, i2c_phase_t phase)
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

static void display_line_state(void)
{
    usart0_send_string_P(PSTR("DISPLAY SSD1306 128x64 @"));
    display_send_hex(CONFIG_DISPLAY_ADDRESS);
    usart0_send_string_P(PSTR(": "));
    display_send_state(snap.state);
    usart0_send_string_P(PSTR(", page "));
    usart0_send_uint32(snap.page);
    usart0_send_string_P(PSTR(" "));
    display_view_send_page_name(snap.page);

    if(snap.page_change){
        usart0_send_string_P(PSTR(" (switch pending)"));
    }
    if(snap.full_request){
        usart0_send_string_P(PSTR(", full refresh pending"));
    }
    usart0_send_newline();
}

static void display_line_frame(void)
{
    usart0_send_string_P(PSTR("DISPLAY FRAME "));

    if(snap.frame == DISPLAY_FRAME_RENDER){
        usart0_send_string_P(PSTR("render row "));
        usart0_send_uint32(snap.render_row);
        usart0_send_string_P(PSTR("/8"));
    } else if(snap.frame == DISPLAY_FRAME_SEND){
        usart0_send_string_P(PSTR("sending "));
        usart0_send_uint32(snap.frame_sent);
        usart0_send_string_P(PSTR("/"));
        usart0_send_uint32(snap.frame_bytes);
        usart0_send_string_P(snap.frame_full ? PSTR(" bytes (full)") : PSTR(" bytes (changed areas)"));
    } else if(snap.state == DISPLAY_RUN){
        usart0_send_string_P(PSTR("idle, next check in "));
        usart0_send_uint32(snap.next_check);
        usart0_send_string_P(PSTR(" ms"));
    } else {
        usart0_send_string_P(PSTR("-"));
    }

    usart0_send_string_P(PSTR(", last "));
    usart0_send_uint32(snap.last_bytes);
    usart0_send_string_P(PSTR(" B in "));
    usart0_send_uint32(snap.last_took);
    usart0_send_string_P(PSTR(" ms"));
    usart0_send_newline();
}

static void display_line_count(void)
{
    usart0_send_string_P(PSTR("DISPLAY FRAMES "));
    usart0_send_uint32(snap.frames);
    usart0_send_string_P(PSTR(" (full "));
    usart0_send_uint32(snap.full_frames);
    usart0_send_string_P(PSTR(", partial "));
    usart0_send_uint32(snap.partial_frames);
    usart0_send_string_P(PSTR("), no change "));
    usart0_send_uint32(snap.unchanged);
    usart0_send_string_P(PSTR(", merged requests "));
    usart0_send_uint32(snap.merged);
    usart0_send_string_P(PSTR(", I2C txns "));
    usart0_send_uint32(snap.transactions);
    usart0_send_newline();
}

static void display_line_full(void)
{
    usart0_send_string_P(PSTR("DISPLAY LAST FULL "));

    if(snap.have_full){
        usart0_send_string_P(PSTR("at "));
        usart0_send_uint32(snap.full_ms);
        usart0_send_string_P(PSTR(" ms, 1024 B in "));
        usart0_send_uint32(snap.full_took);
        usart0_send_string_P(PSTR(" ms"));
    } else {
        usart0_send_string_P(PSTR("none"));
    }
    usart0_send_newline();
}

static void display_line_errors(void)
{
    usart0_send_string_P(PSTR("DISPLAY ERRORS "));
    usart0_send_uint32(snap.errors);
    usart0_send_string_P(PSTR(" (in a row "));
    usart0_send_uint32(snap.consecutive);
    usart0_send_string_P(PSTR("/"));
    usart0_send_uint32(CONFIG_DISPLAY_RETRIES);
    usart0_send_string_P(PSTR("), LAST "));
    display_send_error(snap.error, snap.phase);

    if(snap.errors != 0){
        usart0_send_string_P(PSTR(" during "));
        display_send_state(snap.fail_state);
    }

    if(snap.state == DISPLAY_BACKOFF){
        usart0_send_string_P(PSTR(", retry in "));
        usart0_send_uint32(snap.retry_left);
        usart0_send_string_P(PSTR(" ms"));
    }
    usart0_send_newline();
}

void display_send_report(void)
{
    if(report_part == 1){
        display_line_state();
    } else if(report_part == 2){
        display_line_frame();
    } else if(report_part == 3){
        display_line_count();
    } else if(report_part == 4){
        display_line_full();
    } else {
        display_line_errors();
        report_part = 0;
        return;
    }
    report_part++;
}

bool display_event_pending(void)
{
    return event_pending;
}

void display_send_event(void)
{
    event_pending = false;

    usart0_send_string_P(PSTR("DISPLAY "));

    if(event_type == DISPLAY_EVENT_ON){
        usart0_send_string_P(PSTR("ON @"));
        display_send_hex(CONFIG_DISPLAY_ADDRESS);
        usart0_send_string_P(PSTR(", full refresh 1024 B in "));
        usart0_send_uint32(event_took);
        usart0_send_string_P(PSTR(" ms"));
    } else if(event_type == DISPLAY_EVENT_RETRY){
        usart0_send_string_P(PSTR("RETRY "));
        usart0_send_uint32(event_count);
        usart0_send_string_P(PSTR("/"));
        usart0_send_uint32(CONFIG_DISPLAY_RETRIES);
        usart0_send_string_P(PSTR(" in "));
        usart0_send_uint32(event_backoff);
        usart0_send_string_P(PSTR(" ms: "));
        display_send_error(event_error, event_phase);
        usart0_send_string_P(PSTR(" during "));
        display_send_state(event_state);
    } else if(event_type == DISPLAY_EVENT_FAILED){
        usart0_send_string_P(PSTR("FAILED after "));
        usart0_send_uint32(event_count);
        usart0_send_string_P(PSTR(" errors: "));
        display_send_error(event_error, event_phase);
        usart0_send_string_P(PSTR(", use display on"));
    } else {
        usart0_send_string_P(event_ok ? PSTR("OFF") : PSTR("OFF (AE not acknowledged)"));
    }
    usart0_send_newline();
}
