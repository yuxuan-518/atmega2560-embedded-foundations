#ifndef RANGE_H
#define RANGE_H

#include <stdint.h>
#include <stdbool.h>

typedef enum {
    RANGE_STOPPED,
    RANGE_WAIT,
    RANGE_TRIGGER,
    RANGE_WAIT_RISE,
    RANGE_WAIT_FALL
} range_phase_t;

typedef enum {
    RANGE_NONE,
    RANGE_OK,
    RANGE_NO_ECHO,
    RANGE_ECHO_TIMEOUT,
    RANGE_ECHO_STUCK,
    RANGE_TOO_SHORT,
    RANGE_TOO_LONG
} range_result_t;

typedef enum {
    RANGE_FILTER_OFF,
    RANGE_FILTER_MEAN,
    RANGE_FILTER_MEDIAN
} range_filter_t;

typedef enum {
    RANGE_ALARM_OFF,
    RANGE_ALARM_CLEAR,
    RANGE_ALARM_ACTIVE,
    RANGE_ALARM_FAULT
} range_alarm_t;

void range_init(void);
void range_update(uint32_t now);

void range_start(uint32_t now);
void range_stop(void);
void range_set_filter(range_filter_t filter, uint32_t now);
bool range_set_alarm(uint32_t mm, uint32_t now);

bool range_is_running(void);
range_phase_t range_get_phase(void);
range_result_t range_get_result(void);
uint32_t range_get_pulse(void);
uint16_t range_get_raw(void);
bool range_is_fresh(uint32_t now);
uint16_t range_get_filtered(void);
uint16_t range_get_mean(void);
uint16_t range_get_median(void);
range_filter_t range_get_filter(void);
range_alarm_t range_get_alarm(void);
uint32_t range_get_alarm_mm(void);
uint32_t range_get_count(range_result_t result);
uint32_t range_get_measurements(void);

bool range_alarm_event(void);
void range_send_alarm_event(uint32_t now);
void range_send_status(uint32_t now);

#endif
