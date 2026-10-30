#ifndef SAFETY_H
#define SAFETY_H

#include <stdint.h>
#include <stdbool.h>

void safety_init(void);
void safety_guard_isr(void);
void safety_update(uint32_t now);
bool safety_reset(void);

#endif
