#ifndef FONT5X7_H
#define FONT5X7_H

#include <stdint.h>

#define FONT5X7_FIRST 0x20
#define FONT5X7_LAST 0x7E
#define FONT5X7_WIDTH 5

uint8_t font5x7_column(char c, uint8_t column);
const uint8_t *font5x7_glyph(char c);

#endif
