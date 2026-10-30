#ifndef NH3_H
#define NH3_H

#include <stdint.h>
#include <stdbool.h>

typedef enum {
    NH3_DISABLED,
    NH3_NO_DATA,
    NH3_WARMUP,
    NH3_VALID,
    NH3_STALE
} nh3_state_t;

void nh3_init(uint32_t now);
void nh3_update(uint32_t now);
bool nh3_set_baud(uint32_t baud);
bool nh3_send_active_command(void);
void nh3_set_raw(bool enable);
void nh3_reset_counters(void);

nh3_state_t nh3_get_state(void);
bool nh3_get_ppb(uint32_t *ppb);
void nh3_send_state(nh3_state_t state);
void nh3_send_brief(uint32_t now);
void nh3_send_status(uint32_t now);

#endif
