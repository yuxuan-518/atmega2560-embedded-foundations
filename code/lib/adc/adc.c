#include "adc.h"
#include <avr/io.h>

static bool adc_pending = false;
static uint8_t adc_pending_channel = 0;

void adc_init(void)
{
    ADCSRA = 0;
    ADMUX = (1 << REFS0);
    ADCSRB &= ~(1 << MUX5);
    ADCSRA = (1 << ADEN) |
             (1 << ADPS2) |
             (1 << ADPS1) |
             (1 << ADPS0);
    adc_pending = false;
}

void adc_disable_digital(uint8_t channel)
{
    if(channel < 8){
        DIDR0 |= (1 << channel);
    } else if(channel <= ADC_MAX_CHANNEL){
        DIDR2 |= (1 << (channel - 8));
    }
}

bool adc_busy(void)
{
    return (ADCSRA & (1 << ADSC)) != 0;
}

bool adc_start(uint8_t channel)
{
    if(channel > ADC_MAX_CHANNEL || adc_pending || adc_busy()){
        return false;
    }

    ADMUX = (1 << REFS0) | (channel & 0x07);

    if(channel >= 8){
        ADCSRB |= (1 << MUX5);
    } else {
        ADCSRB &= ~(1 << MUX5);
    }

    ADCSRA |= (1 << ADSC);
    adc_pending = true;
    adc_pending_channel = channel;
    return true;
}

bool adc_read(uint16_t *value)
{
    if(!adc_pending || adc_busy()){
        return false;
    }

    uint8_t low = ADCL;
    uint8_t high = ADCH;

    *value = ((uint16_t)high << 8) | low;
    adc_pending = false;
    return true;
}

bool adc_read_channel(uint16_t *value, uint8_t *channel)
{
    uint8_t owner = adc_pending_channel;

    if(!adc_read(value)){
        return false;
    }

    *channel = owner;
    return true;
}
