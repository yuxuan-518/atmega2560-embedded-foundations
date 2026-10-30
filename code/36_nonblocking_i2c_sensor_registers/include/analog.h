#ifndef ANALOG_H
#define ANALOG_H

#include <stdint.h>
#include <stdbool.h>
#include "adc_scan.h"

bool analog_init(uint32_t now);
void analog_update(uint32_t now);
bool analog_ready(void);
const adc_scan_result_t *analog_get(uint8_t index);
void analog_send_status(uint32_t now);

#endif
