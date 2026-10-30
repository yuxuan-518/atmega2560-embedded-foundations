#ifndef MOTOR_H
#define MOTOR_H

#include <stdint.h>
#include <stdbool.h>

#define MOTOR_START_HZ 100
#define MOTOR_MIN_HZ 100
#define MOTOR_MAX_HZ 5000
#define MOTOR_DEFAULT_TARGET_HZ 1000

#define MOTOR_ACCEL_LOW 500
#define MOTOR_ACCEL_HIGH 2000
#define MOTOR_ACCEL_MAX 20000

#define MOTOR_UPDATE_MS 10

typedef enum {
    MOTOR_STOPPED,
    MOTOR_ACCEL,
    MOTOR_CRUISE,
    MOTOR_DECEL,
    MOTOR_STOPPING,
    MOTOR_REVERSING
} motor_state_t;

void motor_init(void);
void motor_update(uint32_t now);

void motor_start(void);
void motor_stop(void);
void motor_reverse(void);
bool motor_set_target(uint32_t Hz);
bool motor_set_accel(uint32_t Hz_per_s);

motor_state_t motor_get_state(void);
uint32_t motor_get_current_hz(void);
uint32_t motor_get_target_hz(void);
uint32_t motor_get_accel(void);
uint8_t motor_get_direction(void);

#endif
