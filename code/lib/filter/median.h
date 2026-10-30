#ifndef MEDIAN_H
#define MEDIAN_H

#include <stdint.h>
#include <stdbool.h>

#define MEDIAN_MAX_WINDOW 9

typedef struct {
    uint16_t buffer[MEDIAN_MAX_WINDOW];
    uint8_t window;
    uint8_t count;
    uint8_t head;
} median_t;

bool median_init(median_t *m, uint8_t window);
uint16_t median_push(median_t *m, uint16_t value);
uint16_t median_get(const median_t *m);
uint8_t median_count(const median_t *m);

#endif
