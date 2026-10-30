#ifndef COMMAND_POT_H
#define COMMAND_POT_H

#include <stdint.h>

void command_pot_execute(char *args[], uint8_t count);
uint8_t command_pot_config_channel(void);

#endif
