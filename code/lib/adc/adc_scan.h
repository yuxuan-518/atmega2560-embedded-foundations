#ifndef ADC_SCAN_H
#define ADC_SCAN_H

#include <stdint.h>
#include <stdbool.h>

#define ADC_SCAN_MAX_CHANNELS 8

typedef struct {
    uint16_t raw;
    bool valid;
    uint32_t sample_ms;
    uint32_t samples;
} adc_scan_result_t;

bool adc_scan_init(const uint8_t channels[], uint8_t count, uint32_t period_ms, uint32_t now);
bool adc_scan_update(uint32_t now);

uint8_t adc_scan_count(void);
uint8_t adc_scan_channel(uint8_t index);
const adc_scan_result_t *adc_scan_get(uint8_t index);
uint32_t adc_scan_rounds(void);
uint32_t adc_scan_discarded(void);
uint32_t adc_scan_mismatch(void);
uint32_t adc_scan_period(void);

#endif
