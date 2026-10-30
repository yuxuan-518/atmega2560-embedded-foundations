#ifndef DAC_H
#define DAC_H

#include <stdint.h>
#include <stdbool.h>
#include "spi.h"

typedef enum {
    DAC_IDLE,
    DAC_SENDING,
    DAC_LDAC
} dac_state_t;

typedef enum {
    DAC_ERROR_NONE,
    DAC_ERROR_BUS_BUSY,
    DAC_ERROR_SPI
} dac_error_t;

bool dac_init(void);
void dac_update(uint32_t now);
bool dac_set(uint16_t value, uint32_t now);

bool dac_is_ready(void);
dac_state_t dac_get_state(void);
bool dac_has_pending(void);
bool dac_has_value(void);
uint16_t dac_get_value(void);
uint32_t dac_get_sent(void);
uint32_t dac_get_errors(void);
dac_error_t dac_get_error(void);
spi_result_t dac_get_result(void);

void dac_request_report(void);
bool dac_report_pending(void);
void dac_send_report(void);
bool dac_event_pending(void);
void dac_send_event(void);

#endif
