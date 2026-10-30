#include "pot.h"
#include "config.h"
#include "adc.h"
#include "led.h"
#include "usart.h"

static bool valid = false;
static bool pending = false;
static uint16_t raw_value = 0;
static uint32_t last_start = 0;
static uint32_t last_sample_ms = 0;
static uint32_t samples = 0;

uint8_t pot_raw_to_duty(uint16_t raw)
{
    if(raw > ADC_MAX_VALUE){
        raw = ADC_MAX_VALUE;
    }

    return (uint8_t)(((uint32_t)raw * 255UL) / ADC_MAX_VALUE);
}

uint32_t pot_raw_to_mv(uint16_t raw)
{
    if(raw > ADC_MAX_VALUE){
        raw = ADC_MAX_VALUE;
    }

    return ((uint32_t)raw * CONFIG_ADC_VREF_MV) / 1024UL;
}

void pot_init(uint32_t now)
{
    adc_init();
    adc_disable_digital(CONFIG_POT_CHANNEL);
    valid = false;
    pending = false;
    last_start = now - CONFIG_POT_SAMPLE_MS;
}

void pot_update(uint32_t now)
{
    if(pending){
        uint16_t value = 0;

        if(!adc_read(&value)){
            return;
        }

        pending = false;
        raw_value = value;
        valid = true;
        last_sample_ms = now;
        samples++;

        led_pot_input(pot_raw_to_duty(raw_value));
        return;
    }

    if(now - last_start >= CONFIG_POT_SAMPLE_MS){
        if(adc_start(CONFIG_POT_CHANNEL)){
            pending = true;
            last_start = now;
        }
    }
}

bool pot_is_valid(void)
{
    return valid;
}

uint16_t pot_get_raw(void)
{
    return raw_value;
}

uint32_t pot_get_mv(void)
{
    return pot_raw_to_mv(raw_value);
}

void pot_send_status(uint32_t now)
{
    usart0_send_string("ADC: CH ");
    usart0_send_uint32(CONFIG_POT_CHANNEL);
    usart0_send_string(" (A");
    usart0_send_uint32(CONFIG_POT_CHANNEL);
    usart0_send_string("), RAW: ");

    if(valid){
        usart0_send_uint32(raw_value);
        usart0_send_string(", MV: ");
        usart0_send_uint32(pot_raw_to_mv(raw_value));
    } else {
        usart0_send_string("-, MV: -");
    }

    usart0_send_string(", VREF: ");
    usart0_send_uint32(CONFIG_ADC_VREF_MV);
    usart0_send_string(" mV AVCC, VALID: ");
    usart0_send_uint32(valid);
    usart0_send_newline();

    usart0_send_string("ADC LAST: ");

    if(valid){
        usart0_send_uint32(last_sample_ms);
        usart0_send_string(" ms (age ");
        usart0_send_uint32(now - last_sample_ms);
        usart0_send_string(" ms)");
    } else {
        usart0_send_string("-");
    }

    usart0_send_string(", SAMPLES: ");
    usart0_send_uint32(samples);
    usart0_send_string(", PERIOD: ");
    usart0_send_uint32(CONFIG_POT_SAMPLE_MS);
    usart0_send_string(" ms, MAP DUTY: ");

    if(valid){
        usart0_send_uint32(pot_raw_to_duty(raw_value));
    } else {
        usart0_send_string("-");
    }
    usart0_send_newline();
}
