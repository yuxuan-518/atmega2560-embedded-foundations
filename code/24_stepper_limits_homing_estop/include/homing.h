#ifndef HOMING_H
#define HOMING_H

#include <stdint.h>
#include <stdbool.h>

#define HOME_FAST_HZ 400
#define HOME_SLOW_HZ 50
#define HOME_EXIT_HZ 100

#define HOME_SEEK_MAX_STEPS 20000
#define HOME_SEEK_TIMEOUT_MS 60000
#define HOME_SLOW_MAX_STEPS 400
#define HOME_SLOW_TIMEOUT_MS 15000
#define HOME_RELEASE_MAX_STEPS 1000
#define HOME_RELEASE_TIMEOUT_MS 15000
#define HOME_BACKOFF_EXTRA_STEPS 100

void homing_start(void);
void homing_exit(void);
void homing_abort(void);
void homing_invalidate(void);
void homing_update(uint32_t now);
bool homing_busy(void);
bool homing_is_homed(void);
void homing_send_phase(void);

#endif
