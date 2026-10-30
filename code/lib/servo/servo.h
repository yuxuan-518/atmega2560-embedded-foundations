#ifndef SERVO_H
#define SERVO_H

#include <stdint.h>
#include <stdbool.h>

#define SERVO_TICKS_PER_US 2
#define SERVO_MIN_TICKS (500U * SERVO_TICKS_PER_US)
#define SERVO_MAX_TICKS (2500U * SERVO_TICKS_PER_US)
#define SERVO_PERIOD_MIN_US 5000UL
#define SERVO_PERIOD_MAX_US 50000UL

#ifndef SERVO_COMP_TICKS
#define SERVO_COMP_TICKS 5
#endif

bool servo_init(uint32_t period_us);
bool servo_start(uint16_t ticks);
bool servo_set(uint16_t ticks);
void servo_stop(void);

bool servo_is_running(void);
bool servo_is_stopping(void);
bool servo_is_high(void);
uint16_t servo_get_pending(void);
uint16_t servo_get_output(void);
uint32_t servo_get_frames(void);
uint32_t servo_get_period_us(void);

#endif
