#include "counter.h"
#include "config.h"
#include "extint.h"
#include "usart.h"

#include <avr/io.h>
#include <avr/pgmspace.h>

static bool running = false;
static counter_mode_t mode = COUNTER_PULSE;
static bool rising = false;

static bool poll_level = true;
static uint32_t poll_count = 0;
static uint32_t polls = 0;

static bool candidate = false;
static bool stable = false;
static bool armed = true;
static uint32_t candidate_since = 0;
static uint32_t debounce_raw = 0;
static uint32_t presses = 0;

static uint32_t rate_ms = 0;
static uint32_t rate_irq = 0;
static uint32_t rate_poll = 0;
static uint32_t rate_polls = 0;
static uint32_t irq_rate = 0;
static uint32_t poll_rate = 0;
static uint32_t window = 0;
static uint32_t poll_hz = 0;
static bool line_ready = false;

static bool counter_pressed(bool level)
{
    return level == rising;
}

static uint32_t counter_rate_x100(uint32_t delta, uint32_t dt)
{
    if(dt == 0){
        return 0;
    }

    if(delta <= 42949UL){
        return (delta * 100000UL + dt / 2) / dt;
    }

    if(delta <= 4294967UL){
        return ((delta * 1000UL + dt / 2) / dt) * 100UL;
    }

    return (delta / dt) * 100000UL;
}

static void counter_rearm(uint32_t now)
{
    bool level = extint5_pin_high();

    poll_level = level;
    candidate = counter_pressed(level);
    stable = candidate;
    armed = !stable;
    candidate_since = now;
    debounce_raw = extint5_snapshot();

    rate_ms = now;
    rate_irq = extint5_snapshot();
    rate_poll = poll_count;
    rate_polls = polls;
}

static void counter_reset_counts(uint32_t now)
{
    extint5_set_count(0);
    poll_count = 0;
    polls = 0;
    presses = 0;
    irq_rate = 0;
    poll_rate = 0;
    window = 0;
    poll_hz = 0;
    line_ready = false;
    counter_rearm(now);
}

static void counter_debounce(bool level, uint32_t now)
{
    bool pressed = counter_pressed(level);
    uint32_t raw = extint5_snapshot();

    if(raw != debounce_raw){
        debounce_raw = raw;
        candidate = pressed;
        candidate_since = now;
        return;
    }

    if(pressed != candidate){
        candidate = pressed;
        candidate_since = now;
        return;
    }

    if(candidate == stable || now - candidate_since < CONFIG_COUNTER_DEBOUNCE_MS){
        return;
    }

    stable = candidate;

    if(stable && armed){
        presses++;
        armed = false;
    } else if(!stable){
        armed = true;
    }
}

static void counter_rate(uint32_t now)
{
    uint32_t irq = extint5_snapshot();

    window = now - rate_ms;
    irq_rate = counter_rate_x100(irq - rate_irq, window);
    poll_rate = counter_rate_x100(poll_count - rate_poll, window);
    poll_hz = counter_rate_x100(polls - rate_polls, window) / 100;

    rate_ms = now;
    rate_irq = irq;
    rate_poll = poll_count;
    rate_polls = polls;
    line_ready = true;
}

void counter_init(void)
{
    rising = CONFIG_COUNTER_RISING_DEFAULT;
    mode = CONFIG_COUNTER_BUTTON_DEFAULT ? COUNTER_BUTTON : COUNTER_PULSE;
    running = false;

    extint5_init(rising ? EXTINT_RISING : EXTINT_FALLING, CONFIG_COUNTER_PULLUP);
    counter_reset_counts(0);
}

void counter_update(uint32_t now)
{
    if(!running){
        return;
    }

    bool level = extint5_pin_high();
    polls++;

    if(level != poll_level){
        poll_level = level;

        if(level == rising){
            poll_count++;
        }
    }

    if(mode == COUNTER_BUTTON){
        counter_debounce(level, now);
    }

    if(now - rate_ms >= CONFIG_COUNTER_RATE_MS){
        counter_rate(now);
    }
}

void counter_start(uint32_t now)
{
    if(running){
        return;
    }

    counter_rearm(now);
    extint5_enable();
    running = true;
}

void counter_stop(void)
{
    extint5_disable();
    running = false;
    line_ready = false;
}

void counter_clear(uint32_t now)
{
    counter_reset_counts(now);
}

void counter_set_rising(bool new_rising, uint32_t now)
{
    rising = new_rising;
    extint5_set_edge(rising ? EXTINT_RISING : EXTINT_FALLING);
    counter_reset_counts(now);
}

void counter_set_mode(counter_mode_t new_mode, uint32_t now)
{
    mode = new_mode;
    counter_reset_counts(now);
}

bool counter_is_running(void)
{
    return running;
}

counter_mode_t counter_get_mode(void)
{
    return mode;
}

bool counter_is_rising(void)
{
    return rising;
}

uint32_t counter_get_irq(void)
{
    return extint5_snapshot();
}

uint32_t counter_get_poll(void)
{
    return poll_count;
}

uint32_t counter_get_presses(void)
{
    return presses;
}

uint32_t counter_get_irq_rate(void)
{
    return irq_rate;
}

uint32_t counter_get_poll_rate(void)
{
    return poll_rate;
}

uint32_t counter_get_window(void)
{
    return window;
}

uint32_t counter_get_poll_hz(void)
{
    return poll_hz;
}

bool counter_line_ready(void)
{
    return line_ready;
}

static void counter_send_x100(uint32_t value)
{
    usart0_send_uint32(value / 100);
    usart0_send_string_P(PSTR("."));

    if(value % 100 < 10){
        usart0_send_string_P(PSTR("0"));
    }
    usart0_send_uint32(value % 100);
}

void counter_send_line(uint32_t now)
{
    line_ready = false;

    usart0_send_string_P(PSTR("CNT,"));
    usart0_send_uint32(now);
    usart0_send_string_P(PSTR(","));
    usart0_send_uint32(extint5_snapshot());
    usart0_send_string_P(PSTR(","));
    usart0_send_uint32(poll_count);
    usart0_send_string_P(PSTR(","));
    usart0_send_uint32(presses);
    usart0_send_string_P(PSTR(","));
    counter_send_x100(irq_rate);
    usart0_send_string_P(PSTR(","));
    counter_send_x100(poll_rate);
    usart0_send_string_P(PSTR(","));
    usart0_send_uint32(window);
    usart0_send_newline();
}

void counter_send_status(uint32_t now)
{
    uint32_t irq = extint5_snapshot();

    usart0_send_string_P(PSTR("COUNTER D3/INT5: "));

    if(running){
        usart0_send_string_P(PSTR("RUN"));
    } else {
        usart0_send_string_P(PSTR("STOP"));
    }

    if(mode == COUNTER_BUTTON){
        usart0_send_string_P(PSTR(", MODE button"));
    } else {
        usart0_send_string_P(PSTR(", MODE pulse"));
    }

    if(rising){
        usart0_send_string_P(PSTR(", EDGE rising"));
    } else {
        usart0_send_string_P(PSTR(", EDGE falling"));
    }

    if(extint5_pin_high()){
        usart0_send_string_P(PSTR(", PIN high"));
    } else {
        usart0_send_string_P(PSTR(", PIN low"));
    }

    if(extint5_is_enabled()){
        usart0_send_string_P(PSTR(", INT5 on"));
    } else {
        usart0_send_string_P(PSTR(", INT5 off"));
    }

    if(EIMSK & (1 << INT4)){
        usart0_send_string_P(PSTR(", INT4 on"));
    } else {
        usart0_send_string_P(PSTR(", INT4 off"));
    }
    usart0_send_newline();

    usart0_send_string_P(PSTR("COUNTER IRQ "));
    usart0_send_uint32(irq);
    usart0_send_string_P(PSTR(", POLL "));
    usart0_send_uint32(poll_count);
    usart0_send_string_P(PSTR(", IRQ-POLL "));

    if(irq >= poll_count){
        usart0_send_uint32(irq - poll_count);
    } else {
        usart0_send_string_P(PSTR("-"));
        usart0_send_uint32(poll_count - irq);
    }

    if(mode == COUNTER_BUTTON){
        usart0_send_string_P(PSTR(", PRESSES "));
        usart0_send_uint32(presses);
        if(stable){
            usart0_send_string_P(PSTR(" (held)"));
        }
    }
    usart0_send_newline();

    usart0_send_string_P(PSTR("COUNTER RATE IRQ "));
    counter_send_x100(irq_rate);
    usart0_send_string_P(PSTR("/s, POLL "));
    counter_send_x100(poll_rate);
    usart0_send_string_P(PSTR("/s over "));
    usart0_send_uint32(window);
    usart0_send_string_P(PSTR(" ms, LOOP "));
    usart0_send_uint32(poll_hz);
    usart0_send_string_P(PSTR(" polls/s, AGE "));
    usart0_send_uint32(now - rate_ms);
    usart0_send_string_P(PSTR(" ms"));
    usart0_send_newline();
}
