#ifndef ADC_H
#define ADC_H

#include <stdint.h>
#include <stdbool.h>

#define ADC_MAX_CHANNEL 15
#define ADC_MAX_VALUE 1023

void adc_init(void);
void adc_disable_digital(uint8_t channel);
bool adc_start(uint8_t channel);
bool adc_busy(void);
bool adc_read(uint16_t *value);
bool adc_read_channel(uint16_t *value, uint8_t *channel);

#endif
