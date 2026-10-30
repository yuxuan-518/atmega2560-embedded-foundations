#ifndef COMMAND_H
#define COMMAND_H

#include <stdint.h>
#include <stdbool.h>

#define COMMAND_LEN 32
#define COMMAND_INT_MAX 1000000000UL

bool command_receive(char line[COMMAND_LEN]);
uint32_t command_parse_uint32(const char text[]);
bool command_parse_int32(const char text[], int32_t *value);
uint8_t command_split(char line[], char *args[], uint8_t max_args);

#endif
