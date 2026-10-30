#ifndef POSITION_H
#define POSITION_H

#include <stdint.h>
#include <stdbool.h>

bool position_check_target(int32_t target);
bool position_move_steps(int32_t steps, bool report);
bool position_goto(int32_t target, bool report);
void position_update(uint32_t now);
void position_abort(void);
bool position_busy(void);
bool position_last_ok(void);
int32_t position_get_target(void);
uint32_t position_get_remaining(void);

#endif
