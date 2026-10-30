#ifndef COMMAND_H
#define COMMAND_H

#include <stdint.h>
#include <stdbool.h>

#define COMMAND_LEN 16

bool command_receive(char line[COMMAND_LEN]);
uint32_t command_parse_uint32(const char text[]);

#endif
