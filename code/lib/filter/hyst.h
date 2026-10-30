#ifndef HYST_H
#define HYST_H

#include <stdint.h>
#include <stdbool.h>

#define HYST_MIN 0
#define HYST_MAX 255

typedef struct {
    uint8_t threshold;
    uint8_t output;
    bool valid;
} hyst_t;

void hyst_init(hyst_t *h, uint8_t threshold);
void hyst_reset(hyst_t *h);
uint8_t hyst_update(hyst_t *h, uint8_t input);
uint8_t hyst_get(const hyst_t *h);

#endif
