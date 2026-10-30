#ifndef MOTOR_H
#define MOTOR_H

#include <stdint.h>
#include <stdbool.h>

#define MOTOR_DEFAULT_HZ 100
#define MOTOR_MIN_HZ 50
#define MOTOR_MAX_HZ 1000

void motor_init(void);
void motor_start(void);
void motor_stop(void);
bool motor_set_speed(uint32_t Hz);
void motor_change_direction(void);
bool motor_is_running(void);
uint32_t motor_get_speed(void);
uint8_t motor_get_direction(void);

#endif
