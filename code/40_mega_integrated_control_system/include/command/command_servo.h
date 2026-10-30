#ifndef COMMAND_SERVO_H
#define COMMAND_SERVO_H

#include <stdint.h>

void command_servo_execute(char *args[], uint8_t count, uint32_t now);

#endif
