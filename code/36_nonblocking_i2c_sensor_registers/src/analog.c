#include "analog.h"
#include "config.h"
#include "usart.h"

#include <avr/pgmspace.h>

static const uint8_t analog_channels[CONFIG_ADC_COUNT] = {
    CONFIG_ADC_CH0,
    CONFIG_ADC_CH1,
    CONFIG_ADC_CH2
};

static bool ready = false;

bool analog_init(uint32_t now)
{
    ready = adc_scan_init(analog_channels, CONFIG_ADC_COUNT, CONFIG_ADC_SAMPLE_MS, now);
    return ready;
}

void analog_update(uint32_t now)
{
    if(ready){
        adc_scan_update(now);
    }
}

bool analog_ready(void)
{
    return ready;
}

const adc_scan_result_t *analog_get(uint8_t index)
{
    if(!ready){
        return 0;
    }
    return adc_scan_get(index);
}

void analog_send_status(uint32_t now)
{
    for(uint8_t i = 0; i < CONFIG_ADC_COUNT; i++){
        const adc_scan_result_t *result = adc_scan_get(i);

        usart0_send_string_P(PSTR("ADC"));
        usart0_send_uint32(i);
        usart0_send_string_P(PSTR(" A"));
        usart0_send_uint32(adc_scan_channel(i));
        usart0_send_string_P(PSTR(": "));

        if(ready && result->valid){
            usart0_send_string_P(PSTR("RAW "));
            usart0_send_uint32(result->raw);
            usart0_send_string_P(PSTR(", MV "));
            usart0_send_uint32(((uint32_t)result->raw * CONFIG_ADC_VREF_MV) / 1024UL);
            usart0_send_string_P(PSTR(", VALID 1, AGE "));
            usart0_send_uint32(now - result->sample_ms);
            usart0_send_string_P(PSTR(" ms"));
        } else {
            usart0_send_string_P(PSTR("RAW -, MV -, VALID 0, AGE -"));
        }

        usart0_send_string_P(PSTR(", SAMPLES "));
        usart0_send_uint32(ready ? result->samples : 0);
        usart0_send_newline();
    }

    usart0_send_string_P(PSTR("ADC VREF "));
    usart0_send_uint32(CONFIG_ADC_VREF_MV);
    usart0_send_string_P(PSTR(" mV AVCC, PERIOD "));
    usart0_send_uint32(CONFIG_ADC_SAMPLE_MS);
    usart0_send_string_P(PSTR(" ms, ROUNDS "));
    usart0_send_uint32(adc_scan_rounds());
    usart0_send_string_P(PSTR(", DISCARDED "));
    usart0_send_uint32(adc_scan_discarded());
    usart0_send_newline();
}
