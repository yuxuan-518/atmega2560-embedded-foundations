#ifndef FAULT_H
#define FAULT_H

#include <stdint.h>
#include <stdbool.h>

typedef enum {
    FAULT_NONE,
    FAULT_ESTOP,
    FAULT_LIMIT_WIRING,
    FAULT_HOME_NOT_FOUND,
    FAULT_SWITCH_STUCK,
    FAULT_HOME_WRONG_LIMIT,
    FAULT_MOTION_STOPPED
} fault_t;

void fault_set(fault_t fault, uint32_t now);
void fault_clear(void);
bool fault_active(void);
fault_t fault_get(void);
void fault_send_name(fault_t fault);

#endif
