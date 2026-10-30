#ifndef MOTOR_H
#define MOTOR_H

#include <stdint.h>
#include <stdbool.h>

#define MOTOR_UPDATE_MS 10

#define MOTOR_DIR_MIN 0
#define MOTOR_DIR_MAX 1

typedef enum {
    MOTOR_STOPPED,
    MOTOR_ACCEL,
    MOTOR_CRUISE,
    MOTOR_DECEL,
    MOTOR_STOPPING,
    MOTOR_REVERSING,
    MOTOR_JOG
} motor_state_t;

void motor_init(void);
void motor_update(uint32_t now);

bool motor_start(void);
bool motor_move(int32_t steps);
bool motor_jog(uint8_t dir, uint32_t Hz);
void motor_stop(void);
void motor_stop_now(void);
void motor_reverse(void);
bool motor_set_direction(uint8_t dir);
void motor_refresh_target(void);
void motor_set_position(int32_t steps);

motor_state_t motor_get_state(void);
bool motor_is_moving(void);
bool motor_is_positioning(void);
uint32_t motor_get_current_hz(void);
uint32_t motor_get_remaining(void);
uint8_t motor_get_direction(void);
int32_t motor_get_position(void);

#endif
