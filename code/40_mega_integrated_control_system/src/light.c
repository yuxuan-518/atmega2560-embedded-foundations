#include "light.h"
#include "config.h"
#include "analog.h"
#include "adc.h"
#include "avg.h"
#include "led.h"
#include "usart.h"

#include <avr/pgmspace.h>

static avg_t avg;
static uint16_t on_level = CONFIG_LIGHT_ON_DEFAULT;
static uint16_t off_level = CONFIG_LIGHT_OFF_DEFAULT;
static bool invert = CONFIG_LIGHT_INVERT_DEFAULT;

static light_data_t data = LIGHT_NO_DATA;
static bool lamp_on = false;
static bool have_avg = false;
static uint32_t seen_samples = 0;
static bool have_sample = false;
static uint32_t last_sample_ms = 0;
static uint16_t raw_value = 0;
static uint16_t avg_value = 0;
static uint16_t level = 0;
static uint32_t switches = 0;

static void light_output(void)
{
    if(data == LIGHT_OK && lamp_on){
        led_auto_input(255);
    } else {
        led_auto_input(0);
    }
}

static void light_decide(void)
{
    level = invert ? (ADC_MAX_VALUE - avg_value) : avg_value;

    bool next = lamp_on;

    if(level <= on_level){
        next = true;
    } else if(level >= off_level){
        next = false;
    }

    if(next != lamp_on){
        lamp_on = next;
        switches++;
    }

    light_output();
}

static void light_restart(void)
{
    avg_init(&avg, avg_window(&avg));
    have_avg = false;
    lamp_on = false;
}

void light_init(void)
{
    avg_init(&avg, CONFIG_LIGHT_WINDOW_DEFAULT);
    on_level = CONFIG_LIGHT_ON_DEFAULT;
    off_level = CONFIG_LIGHT_OFF_DEFAULT;
    invert = CONFIG_LIGHT_INVERT_DEFAULT;
    data = LIGHT_NO_DATA;
    lamp_on = false;
    have_avg = false;
    seen_samples = 0;
    have_sample = false;
    last_sample_ms = 0;
    switches = 0;
}

void light_update(uint32_t now)
{
    const adc_scan_result_t *result = analog_get(CONFIG_LIGHT_INDEX);

    if(result == 0 || !result->valid){
        if(data != LIGHT_NO_DATA){
            data = LIGHT_NO_DATA;
            light_restart();
        }
        light_output();
        return;
    }

    if(result->samples != seen_samples){
        if(have_sample && result->sample_ms - last_sample_ms > CONFIG_LIGHT_STALE_MS){
            light_restart();
        }

        have_sample = true;
        last_sample_ms = result->sample_ms;
        seen_samples = result->samples;
        raw_value = result->raw;
        avg_value = avg_push(&avg, raw_value);
        have_avg = true;
        data = LIGHT_OK;
        light_decide();
        return;
    }

    if(now - result->sample_ms > CONFIG_LIGHT_STALE_MS){
        if(data != LIGHT_STALE){
            data = LIGHT_STALE;
            light_restart();
        }
        light_output();
    }
}

bool light_set_thresholds(uint16_t new_on, uint16_t new_off)
{
    if(new_on >= new_off || new_off > ADC_MAX_VALUE){
        return false;
    }

    on_level = new_on;
    off_level = new_off;

    if(data == LIGHT_OK && have_avg){
        light_decide();
    }
    return true;
}

bool light_set_window(uint8_t window)
{
    if(!avg_init(&avg, window)){
        return false;
    }

    if(data == LIGHT_OK){
        avg_value = avg_push(&avg, raw_value);
        light_decide();
    } else {
        have_avg = false;
    }
    return true;
}

void light_set_invert(bool new_invert)
{
    invert = new_invert;

    if(data == LIGHT_OK && have_avg){
        light_decide();
    }
}

light_data_t light_get_data(void)
{
    return data;
}

bool light_lamp_on(void)
{
    return data == LIGHT_OK && lamp_on;
}

uint16_t light_get_level(void)
{
    return level;
}

void light_send_status(uint32_t now)
{
    const adc_scan_result_t *result = analog_get(CONFIG_LIGHT_INDEX);

    usart0_send_string_P(PSTR("LIGHT A"));
    usart0_send_uint32(CONFIG_ADC_CH2);
    usart0_send_string_P(PSTR(": "));

    if(data == LIGHT_NO_DATA){
        usart0_send_string_P(PSTR("NO_DATA"));
    } else if(data == LIGHT_STALE){
        usart0_send_string_P(PSTR("STALE"));
    } else {
        usart0_send_string_P(PSTR("OK"));
    }

    if(have_avg){
        usart0_send_string_P(PSTR(", RAW "));
        usart0_send_uint32(raw_value);
        usart0_send_string_P(PSTR(", AVG "));
        usart0_send_uint32(avg_value);
        usart0_send_string_P(PSTR(", LEVEL "));
        usart0_send_uint32(level);
    }

    usart0_send_string_P(PSTR(", AGE "));
    if(result != 0 && result->valid){
        usart0_send_uint32(now - result->sample_ms);
        usart0_send_string_P(PSTR(" ms"));
    } else {
        usart0_send_string_P(PSTR("-"));
    }
    usart0_send_newline();

    usart0_send_string_P(PSTR("LIGHT ON <= "));
    usart0_send_uint32(on_level);
    usart0_send_string_P(PSTR(", OFF >= "));
    usart0_send_uint32(off_level);
    usart0_send_string_P(PSTR(", WIN "));
    usart0_send_uint32(avg_count(&avg));
    usart0_send_string_P(PSTR("/"));
    usart0_send_uint32(avg_window(&avg));

    if(invert){
        usart0_send_string_P(PSTR(", POLARITY invert"));
    } else {
        usart0_send_string_P(PSTR(", POLARITY normal"));
    }

    if(light_lamp_on()){
        usart0_send_string_P(PSTR(", LAMP ON"));
    } else {
        usart0_send_string_P(PSTR(", LAMP OFF"));
    }

    usart0_send_string_P(PSTR(", SWITCHES "));
    usart0_send_uint32(switches);

    if(led_get_mode() == LED_AUTO){
        usart0_send_string_P(PSTR(" (active)"));
    } else {
        usart0_send_string_P(PSTR(" (LED not in auto mode)"));
    }
    usart0_send_newline();
}
