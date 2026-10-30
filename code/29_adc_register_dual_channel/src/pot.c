#include "pot.h"
#include "config.h"
#include "adc.h"
#include "adc_scan.h"
#include "led.h"
#include "usart.h"

static const uint8_t pot_channels[CONFIG_ADC_COUNT] = {
    CONFIG_ADC_CH0,
    CONFIG_ADC_CH1
};

static bool ready = false;
static uint8_t source = CONFIG_POT_DEFAULT_SOURCE;
static uint32_t applied_samples = 0;

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

bool pot_init(uint32_t now)
{
    ready = adc_scan_init(pot_channels, CONFIG_ADC_COUNT, CONFIG_ADC_SAMPLE_MS, now);
    source = CONFIG_POT_DEFAULT_SOURCE;
    applied_samples = 0;
    return ready;
}

void pot_update(uint32_t now)
{
    if(!ready){
        return;
    }

    adc_scan_update(now);

    const adc_scan_result_t *result = adc_scan_get(source);

    if(result->valid && result->samples != applied_samples){
        applied_samples = result->samples;
        led_pot_input(pot_raw_to_duty(result->raw));
    }
}

bool pot_set_source(uint8_t new_source)
{
    if(new_source >= CONFIG_ADC_COUNT){
        return false;
    }

    source = new_source;
    applied_samples = adc_scan_get(source)->samples;
    return true;
}

uint8_t pot_get_source(void)
{
    return source;
}

bool pot_is_valid(uint8_t s)
{
    const adc_scan_result_t *result = adc_scan_get(s);
    return ready && result != 0 && result->valid;
}

uint16_t pot_get_raw(uint8_t s)
{
    const adc_scan_result_t *result = adc_scan_get(s);

    if(result == 0){
        return 0;
    }
    return result->raw;
}

bool pot_source_valid(void)
{
    return pot_is_valid(source);
}

uint8_t pot_source_duty(void)
{
    return pot_raw_to_duty(pot_get_raw(source));
}

void pot_send_status(uint32_t now)
{
    for(uint8_t i = 0; i < CONFIG_ADC_COUNT; i++){
        const adc_scan_result_t *result = adc_scan_get(i);

        usart0_send_string("ADC");
        usart0_send_uint32(i);
        usart0_send_string(" A");
        usart0_send_uint32(adc_scan_channel(i));
        usart0_send_string(": ");

        if(result->valid){
            usart0_send_string("RAW ");
            usart0_send_uint32(result->raw);
            usart0_send_string(", MV ");
            usart0_send_uint32(pot_raw_to_mv(result->raw));
            usart0_send_string(", VALID 1, AGE ");
            usart0_send_uint32(now - result->sample_ms);
            usart0_send_string(" ms");
        } else {
            usart0_send_string("RAW -, MV -, VALID 0, AGE -");
        }

        usart0_send_string(", SAMPLES ");
        usart0_send_uint32(result->samples);
        usart0_send_newline();
    }

    usart0_send_string("ADC VREF ");
    usart0_send_uint32(CONFIG_ADC_VREF_MV);
    usart0_send_string(" mV AVCC, PERIOD ");
    usart0_send_uint32(CONFIG_ADC_SAMPLE_MS);
    usart0_send_string(" ms, ROUNDS ");
    usart0_send_uint32(adc_scan_rounds());
    usart0_send_string(", DISCARDED ");
    usart0_send_uint32(adc_scan_discarded());
    usart0_send_string(", LED SOURCE A");
    usart0_send_uint32(adc_scan_channel(source));

    if(led_get_mode() == LED_POT){
        usart0_send_string(" (active)");
    } else {
        usart0_send_string(" (LED not in pot mode)");
    }
    usart0_send_newline();
}
