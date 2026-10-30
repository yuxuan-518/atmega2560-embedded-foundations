#ifndef TIMER_H
#define TIMER_H

#include <stdint.h>
#include <stdbool.h>

bool timer1_init(uint32_t Hz);
bool timer1_set_hz(uint32_t Hz);
bool timer1_update_hz(uint32_t Hz);
bool timer1_start(void);
void timer1_stop(void);
bool timer1_is_running(void);

void timer1_lock(void);
void timer1_unlock(void);
bool timer1_is_locked(void);

void timer1_set_count_level(uint8_t level);
uint32_t timer1_take_steps(void);

void timer1_set_budget(uint32_t steps);
uint32_t timer1_get_budget(void);

bool timer3_init(uint32_t Hz);

#endif
