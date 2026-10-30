#ifndef LOOPBACK_H
#define LOOPBACK_H

#include <stdint.h>
#include <stdbool.h>
#include "spi.h"

typedef enum {
    LOOP_IDLE,
    LOOP_RUNNING,
    LOOP_PASS,
    LOOP_FAIL,
    LOOP_ERROR,
    LOOP_STOPPED
} loop_state_t;

typedef enum {
    LOOP_P00,
    LOOP_PFF,
    LOOP_P55,
    LOOP_PAA,
    LOOP_INC,
    LOOP_RANDOM,
    LOOP_PATTERNS
} loop_pattern_t;

typedef enum {
    LOOP_START_OK,
    LOOP_START_RUNNING,
    LOOP_START_STOPPING
} loop_start_t;

typedef enum {
    LOOP_CFG_OK,
    LOOP_CFG_BUSY,
    LOOP_CFG_BAD
} loop_cfg_t;

void loopback_init(void);
void loopback_update(uint32_t now);

loop_start_t loopback_start(uint32_t now);
bool loopback_stopping(void);
bool loopback_stop(void);
bool loopback_is_running(void);
loop_cfg_t loopback_set_mode(uint8_t mode);
loop_cfg_t loopback_set_clock(uint32_t Hz);
void loopback_get_settings(spi_settings_t *out);

loop_state_t loopback_get_state(void);
uint16_t loopback_get_sent(void);
uint16_t loopback_get_verified(void);
uint16_t loopback_get_total(void);
uint32_t loopback_get_runs(void);
uint32_t loopback_get_passes(void);
uint8_t loopback_get_bad(void);
uint8_t loopback_byte(uint8_t pattern, uint16_t index, uint16_t *lfsr);

void loopback_request_report(void);
bool loopback_report_pending(void);
void loopback_send_report(void);
bool loopback_event_pending(void);
void loopback_send_event(void);

#endif
