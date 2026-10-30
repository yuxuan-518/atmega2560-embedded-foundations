#ifndef TIMER_H
#define TIMER_H

#include <stdint.h>
#include <stdbool.h>

bool timer1_init(uint32_t Hz);
bool timer1_set_hz(uint32_t Hz);
bool timer3_init(uint32_t Hz);

#endif
