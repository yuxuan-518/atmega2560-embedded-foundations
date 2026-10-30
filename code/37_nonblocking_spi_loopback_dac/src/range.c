#include "range.h"
#include "config.h"
#include "ultrasonic.h"
#include "ustimer.h"
#include "avg.h"
#include "median.h"
#include "usart.h"

#include <avr/pgmspace.h>

static range_phase_t phase = RANGE_STOPPED;
static uint32_t start_ms = 0;
static uint32_t trig_us = 0;

static range_result_t last_result = RANGE_NONE;
static uint32_t last_pulse = 0;
static uint16_t last_raw = 0;
static uint32_t last_ms = 0;

static bool have_valid = false;
static uint16_t valid_mm = 0;
static uint32_t valid_ms = 0;

static avg_t mean;
static median_t median;
static range_filter_t filter = RANGE_FILTER_MEDIAN;

static uint32_t alarm_mm = CONFIG_RANGE_ALARM_DEFAULT_MM;
static range_alarm_t alarm = RANGE_ALARM_OFF;
static range_alarm_t reported_alarm = RANGE_ALARM_OFF;

static uint32_t counts[RANGE_TOO_LONG + 1];
static uint32_t measurements = 0;

static void range_reset_filters(void)
{
    avg_init(&mean, CONFIG_RANGE_FILTER_N);
    median_init(&median, CONFIG_RANGE_FILTER_N);
}

static void range_finish(range_result_t result, uint32_t pulse, uint32_t now)
{
    last_result = result;
    last_pulse = pulse;
    last_ms = now;
    counts[result]++;
    measurements++;
    phase = RANGE_WAIT;

    if(result != RANGE_OK){
        last_raw = 0;
        return;
    }

    last_raw = (uint16_t)((pulse * CONFIG_RANGE_SOUND_MPS + 1000UL) / 2000UL);

    if(!have_valid || now - valid_ms > CONFIG_RANGE_STALE_MS){
        range_reset_filters();
    }

    avg_push(&mean, last_raw);
    median_push(&median, last_raw);
    valid_mm = last_raw;
    valid_ms = now;
    have_valid = true;
}

static void range_check_pulse(uint32_t pulse, uint32_t now)
{
    if(pulse < CONFIG_RANGE_MIN_US){
        range_finish(RANGE_TOO_SHORT, pulse, now);
    } else if(pulse > CONFIG_RANGE_MAX_US){
        range_finish(RANGE_TOO_LONG, pulse, now);
    } else {
        range_finish(RANGE_OK, pulse, now);
    }
}

static void range_update_alarm(uint32_t now)
{
    if(phase == RANGE_STOPPED || alarm_mm == 0){
        alarm = RANGE_ALARM_OFF;
        return;
    }

    if(!range_is_fresh(now)){
        alarm = RANGE_ALARM_FAULT;
        return;
    }

    uint16_t distance = range_get_filtered();

    if(alarm == RANGE_ALARM_ACTIVE){
        if(distance >= alarm_mm + CONFIG_RANGE_ALARM_HYST_MM){
            alarm = RANGE_ALARM_CLEAR;
        }
    } else if(distance < alarm_mm){
        alarm = RANGE_ALARM_ACTIVE;
    } else {
        alarm = RANGE_ALARM_CLEAR;
    }
}

void range_init(void)
{
    phase = RANGE_STOPPED;
    last_result = RANGE_NONE;
    have_valid = false;
    filter = RANGE_FILTER_MEDIAN;
    alarm_mm = CONFIG_RANGE_ALARM_DEFAULT_MM;
    alarm = RANGE_ALARM_OFF;
    reported_alarm = RANGE_ALARM_OFF;
    measurements = 0;

    for(uint8_t i = 0; i <= RANGE_TOO_LONG; i++){
        counts[i] = 0;
    }

    range_reset_filters();
    ultrasonic_trig(false);
    ultrasonic_disarm();
}

void range_update(uint32_t now)
{
    ultrasonic_echo_t echo;
    uint32_t us = 0;

    switch(phase){
        case RANGE_STOPPED:
            break;

        case RANGE_WAIT:
            if(now - start_ms < CONFIG_RANGE_PERIOD_MS){
                break;
            }

            start_ms = now;

            if(ultrasonic_echo_high()){
                range_finish(RANGE_ECHO_STUCK, 0, now);
                break;
            }

            ultrasonic_arm();
            ultrasonic_trig(true);
            trig_us = ustimer_now();
            phase = RANGE_TRIGGER;
            break;

        case RANGE_TRIGGER:
            if(ustimer_now() - trig_us < CONFIG_RANGE_TRIG_US){
                break;
            }

            ultrasonic_trig(false);
            trig_us = ustimer_now();
            phase = RANGE_WAIT_RISE;
            break;

        case RANGE_WAIT_RISE:
            ultrasonic_get_echo(&echo);
            us = ustimer_now();

            if(echo.rise_seen && echo.rise_us - trig_us > CONFIG_RANGE_RISE_TIMEOUT_US){
                ultrasonic_disarm();
                range_finish(RANGE_NO_ECHO, 0, now);
            } else if(echo.rise_seen){
                phase = RANGE_WAIT_FALL;
            } else if(us - trig_us > CONFIG_RANGE_RISE_TIMEOUT_US){
                ultrasonic_disarm();
                range_finish(RANGE_NO_ECHO, 0, now);
            }
            break;

        case RANGE_WAIT_FALL:
            ultrasonic_get_echo(&echo);
            us = ustimer_now();

            if(echo.fall_seen && echo.fall_us - echo.rise_us > CONFIG_RANGE_ECHO_TIMEOUT_US){
                range_finish(RANGE_ECHO_TIMEOUT, echo.fall_us - echo.rise_us, now);
            } else if(echo.fall_seen){
                range_check_pulse(echo.fall_us - echo.rise_us, now);
            } else if(us - echo.rise_us > CONFIG_RANGE_ECHO_TIMEOUT_US){
                ultrasonic_disarm();
                range_finish(RANGE_ECHO_TIMEOUT, us - echo.rise_us, now);
            }
            break;
    }

    range_update_alarm(now);
}

void range_start(uint32_t now)
{
    if(phase != RANGE_STOPPED){
        return;
    }

    ultrasonic_trig(false);
    ultrasonic_disarm();
    range_reset_filters();
    have_valid = false;
    start_ms = now - CONFIG_RANGE_PERIOD_MS;
    phase = RANGE_WAIT;
    alarm = RANGE_ALARM_CLEAR;
    range_update_alarm(now);
}

void range_stop(void)
{
    ultrasonic_trig(false);
    ultrasonic_disarm();
    phase = RANGE_STOPPED;
    alarm = RANGE_ALARM_OFF;
}

void range_set_filter(range_filter_t new_filter, uint32_t now)
{
    filter = new_filter;
    range_update_alarm(now);
}

bool range_set_alarm(uint32_t mm, uint32_t now)
{
    if(mm != 0 && (mm < CONFIG_RANGE_MIN_MM || mm > CONFIG_RANGE_MAX_MM)){
        return false;
    }

    alarm_mm = mm;

    if(alarm != RANGE_ALARM_FAULT){
        alarm = RANGE_ALARM_CLEAR;
    }
    range_update_alarm(now);
    return true;
}

bool range_is_running(void)
{
    return phase != RANGE_STOPPED;
}

range_phase_t range_get_phase(void)
{
    return phase;
}

range_result_t range_get_result(void)
{
    return last_result;
}

uint32_t range_get_pulse(void)
{
    return last_pulse;
}

uint16_t range_get_raw(void)
{
    return last_raw;
}

bool range_is_fresh(uint32_t now)
{
    return have_valid && now - valid_ms <= CONFIG_RANGE_STALE_MS;
}

uint16_t range_get_mean(void)
{
    return avg_get(&mean);
}

uint16_t range_get_median(void)
{
    return median_get(&median);
}

uint16_t range_get_filtered(void)
{
    if(filter == RANGE_FILTER_MEAN){
        return avg_get(&mean);
    }
    if(filter == RANGE_FILTER_MEDIAN){
        return median_get(&median);
    }
    return valid_mm;
}

range_filter_t range_get_filter(void)
{
    return filter;
}

range_alarm_t range_get_alarm(void)
{
    return alarm;
}

uint32_t range_get_alarm_mm(void)
{
    return alarm_mm;
}

uint32_t range_get_count(range_result_t result)
{
    if(result > RANGE_TOO_LONG){
        return 0;
    }
    return counts[result];
}

uint32_t range_get_measurements(void)
{
    return measurements;
}

bool range_alarm_event(void)
{
    return alarm != reported_alarm;
}

static void range_send_alarm_name(range_alarm_t state)
{
    if(state == RANGE_ALARM_OFF){
        usart0_send_string_P(PSTR("OFF"));
    } else if(state == RANGE_ALARM_CLEAR){
        usart0_send_string_P(PSTR("CLEAR"));
    } else if(state == RANGE_ALARM_ACTIVE){
        usart0_send_string_P(PSTR("ACTIVE"));
    } else {
        usart0_send_string_P(PSTR("FAULT"));
    }
}

void range_send_alarm_event(uint32_t now)
{
    reported_alarm = alarm;

    usart0_send_string_P(PSTR("["));
    usart0_send_uint32(now);
    usart0_send_string_P(PSTR(" ms] RANGE ALARM "));
    range_send_alarm_name(alarm);

    if(alarm == RANGE_ALARM_ACTIVE || alarm == RANGE_ALARM_CLEAR){
        usart0_send_string_P(PSTR(", "));
        usart0_send_uint32(range_get_filtered());
        usart0_send_string_P(PSTR(" mm"));
    }
    usart0_send_newline();
}

static void range_send_result_name(range_result_t result)
{
    if(result == RANGE_NONE){
        usart0_send_string_P(PSTR("NONE"));
    } else if(result == RANGE_OK){
        usart0_send_string_P(PSTR("OK"));
    } else if(result == RANGE_NO_ECHO){
        usart0_send_string_P(PSTR("NO_ECHO"));
    } else if(result == RANGE_ECHO_TIMEOUT){
        usart0_send_string_P(PSTR("ECHO_TIMEOUT"));
    } else if(result == RANGE_ECHO_STUCK){
        usart0_send_string_P(PSTR("ECHO_STUCK"));
    } else if(result == RANGE_TOO_SHORT){
        usart0_send_string_P(PSTR("TOO_SHORT"));
    } else {
        usart0_send_string_P(PSTR("TOO_LONG"));
    }
}

void range_send_status(uint32_t now)
{
    usart0_send_string_P(PSTR("RANGE: "));

    if(phase == RANGE_STOPPED){
        usart0_send_string_P(PSTR("STOPPED"));
    } else if(phase == RANGE_WAIT){
        usart0_send_string_P(PSTR("WAIT"));
    } else if(phase == RANGE_TRIGGER){
        usart0_send_string_P(PSTR("TRIGGER"));
    } else if(phase == RANGE_WAIT_RISE){
        usart0_send_string_P(PSTR("WAIT_RISE"));
    } else {
        usart0_send_string_P(PSTR("WAIT_FALL"));
    }

    usart0_send_string_P(PSTR(", LAST "));
    range_send_result_name(last_result);

    if(last_result != RANGE_NONE){
        usart0_send_string_P(PSTR(", PULSE "));
        usart0_send_uint32(last_pulse);
        usart0_send_string_P(PSTR(" us"));

        if(last_result == RANGE_OK){
            usart0_send_string_P(PSTR(", RAW "));
            usart0_send_uint32(last_raw);
            usart0_send_string_P(PSTR(" mm"));
        }

        usart0_send_string_P(PSTR(", "));
        usart0_send_uint32(now - last_ms);
        usart0_send_string_P(PSTR(" ms ago"));
    }
    usart0_send_newline();

    usart0_send_string_P(PSTR("RANGE VALID "));

    if(have_valid){
        usart0_send_uint32(valid_mm);
        usart0_send_string_P(PSTR(" mm, AGE "));
        usart0_send_uint32(now - valid_ms);

        if(range_is_fresh(now)){
            usart0_send_string_P(PSTR(" ms FRESH"));
        } else {
            usart0_send_string_P(PSTR(" ms STALE"));
        }

        usart0_send_string_P(PSTR(", FILTER "));

        if(filter == RANGE_FILTER_MEAN){
            usart0_send_string_P(PSTR("mean "));
        } else if(filter == RANGE_FILTER_MEDIAN){
            usart0_send_string_P(PSTR("median "));
        } else {
            usart0_send_string_P(PSTR("off "));
        }

        usart0_send_uint32(range_get_filtered());
        usart0_send_string_P(PSTR(" mm (MEAN "));
        usart0_send_uint32(avg_get(&mean));
        usart0_send_string_P(PSTR(", MEDIAN "));
        usart0_send_uint32(median_get(&median));
        usart0_send_string_P(PSTR(", N "));
        usart0_send_uint32(median_count(&median));
        usart0_send_string_P(PSTR(")"));
    } else {
        usart0_send_string_P(PSTR("none"));
    }
    usart0_send_newline();

    usart0_send_string_P(PSTR("RANGE ALARM < "));
    usart0_send_uint32(alarm_mm);
    usart0_send_string_P(PSTR(" mm (+"));
    usart0_send_uint32(CONFIG_RANGE_ALARM_HYST_MM);
    usart0_send_string_P(PSTR("): "));
    range_send_alarm_name(alarm);
    usart0_send_string_P(PSTR(", OK "));
    usart0_send_uint32(counts[RANGE_OK]);
    usart0_send_string_P(PSTR(", NO_ECHO "));
    usart0_send_uint32(counts[RANGE_NO_ECHO]);
    usart0_send_string_P(PSTR(", TIMEOUT "));
    usart0_send_uint32(counts[RANGE_ECHO_TIMEOUT]);
    usart0_send_string_P(PSTR(", STUCK "));
    usart0_send_uint32(counts[RANGE_ECHO_STUCK]);
    usart0_send_string_P(PSTR(", SHORT "));
    usart0_send_uint32(counts[RANGE_TOO_SHORT]);
    usart0_send_string_P(PSTR(", LONG "));
    usart0_send_uint32(counts[RANGE_TOO_LONG]);
    usart0_send_string_P(PSTR(", STRAY "));
    usart0_send_uint32(ultrasonic_get_stray());
    usart0_send_newline();

    usart0_send_string_P(PSTR("RANGE SOUND "));
    usart0_send_uint32(CONFIG_RANGE_SOUND_MPS);
    usart0_send_string_P(PSTR(" m/s fixed, no temperature compensation, timer 4 us"));
    usart0_send_newline();
}
