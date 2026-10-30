#ifndef REPEAT_H
#define REPEAT_H

#include <stdint.h>
#include <stdbool.h>

bool repeat_start(int32_t a, int32_t b, int32_t cycles);
void repeat_update(uint32_t now);
void repeat_abort(void);
bool repeat_busy(void);

#endif
