#ifndef COUNTER_H
#define COUNTER_H

#include <stdint.h>
#include <stdbool.h>

typedef enum {
    COUNTER_PULSE,
    COUNTER_BUTTON
} counter_mode_t;

void counter_init(void);
void counter_update(uint32_t now);

void counter_start(uint32_t now);
void counter_stop(void);
void counter_clear(uint32_t now);
void counter_set_rising(bool rising, uint32_t now);
void counter_set_mode(counter_mode_t new_mode, uint32_t now);

bool counter_is_running(void);
counter_mode_t counter_get_mode(void);
bool counter_is_rising(void);
uint32_t counter_get_irq(void);
uint32_t counter_get_poll(void);
uint32_t counter_get_presses(void);
uint32_t counter_get_irq_rate(void);
uint32_t counter_get_poll_rate(void);
uint32_t counter_get_window(void);
uint32_t counter_get_poll_hz(void);
bool counter_line_ready(void);
void counter_send_line(uint32_t now);
void counter_send_status(uint32_t now);

#endif
