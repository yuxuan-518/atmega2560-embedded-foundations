#ifndef AUTORUN_H
#define AUTORUN_H

#include <stdint.h>
#include <stdbool.h>
#include "runlog.h"

typedef enum {
    AUTO_IDLE,
    AUTO_RUNNING,
    AUTO_ENDING
} auto_phase_t;

typedef enum {
    AUTO_START_OK,
    AUTO_START_ALREADY,
    AUTO_START_FAULT,
    AUTO_START_BUSY,
    AUTO_START_NOT_HOMED,
    AUTO_START_NO_POINTS,
    AUTO_START_RANGE,
    AUTO_START_LIMIT,
    AUTO_START_MOTION
} auto_start_t;

typedef enum {
    AUTO_SET_OK,
    AUTO_SET_BUSY,
    AUTO_SET_SAME,
    AUTO_SET_RANGE
} auto_set_t;

void autorun_init(void);
void autorun_update(uint32_t now);

auto_set_t autorun_set_points(int32_t a, int32_t b);
auto_set_t autorun_set_cycles(int32_t cycles);
auto_start_t autorun_start(uint32_t now);
void autorun_stop(void);
void autorun_abort(void);

auto_phase_t autorun_get_phase(void);
bool autorun_busy(void);
bool autorun_points_set(void);
int32_t autorun_get_a(void);
int32_t autorun_get_b(void);
uint16_t autorun_get_cycles(void);
uint16_t autorun_get_leg(void);
uint16_t autorun_get_legs_done(void);
uint16_t autorun_get_legs_total(void);
uint16_t autorun_get_cycles_done(void);
int32_t autorun_get_target(void);
bool autorun_target_is_b(void);
uint32_t autorun_get_elapsed(uint32_t now);
bool autorun_point_ok(int32_t point);

#endif
