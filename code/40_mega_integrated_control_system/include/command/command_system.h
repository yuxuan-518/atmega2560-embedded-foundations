#ifndef COMMAND_SYSTEM_H
#define COMMAND_SYSTEM_H

#include <stdint.h>

void command_system_execute(char *args[], uint8_t count, uint32_t now);

#endif
