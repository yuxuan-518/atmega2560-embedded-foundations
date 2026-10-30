#ifndef USTIMER_H
#define USTIMER_H

#include <stdint.h>
#include <stdbool.h>

#define USTIMER_US_PER_TICK 4UL

void ustimer_init(void);
uint32_t ustimer_now(void);
uint32_t ustimer_now_isr(void);

#endif
