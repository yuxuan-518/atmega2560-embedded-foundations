#ifndef ESTOP_H
#define ESTOP_H

#include <stdbool.h>

#define ESTOP_ACTIVE_LEVEL 0

void estop_init(void);
bool estop_input_active(void);
bool estop_is_latched(void);
void estop_latch(void);
void estop_clear(void);

#endif
