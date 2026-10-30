#ifndef CALIB_H
#define CALIB_H

#include <stdint.h>
#include <stdbool.h>

typedef struct {
    uint16_t low;
    uint16_t high;
    uint16_t max;
} calib_t;

bool calib_init(calib_t *c, uint16_t max);
bool calib_set(calib_t *c, uint16_t low, uint16_t high);
uint16_t calib_apply(const calib_t *c, uint16_t value);

#endif
