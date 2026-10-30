#ifndef COMMAND_MOTOR_H
#define COMMAND_MOTOR_H

#include <stdbool.h>
#include "motor.h"

void command_motor_execute(const char line[]);
void command_motor_send_status(void);
void command_motor_send_state(motor_state_t state);

#endif
