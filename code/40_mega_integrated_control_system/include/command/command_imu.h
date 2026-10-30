#ifndef COMMAND_IMU_H
#define COMMAND_IMU_H

#include <stdint.h>

void command_imu_execute(char *args[], uint8_t count, uint32_t now);

#endif
