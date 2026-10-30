#ifndef HOMING_H
#define HOMING_H

#include <stdint.h>
#include <stdbool.h>

void homing_start(void);
void homing_exit(void);
void homing_abort(void);
void homing_invalidate(void);
void homing_update(uint32_t now);
bool homing_busy(void);
bool homing_is_homed(void);
void homing_send_phase(void);

#endif
