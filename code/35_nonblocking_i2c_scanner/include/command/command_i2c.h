#ifndef COMMAND_I2C_H
#define COMMAND_I2C_H

#include <stdint.h>

void command_i2c_execute(char *args[], uint8_t count, uint32_t now);

#endif
