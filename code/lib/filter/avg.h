#ifndef AVG_H
#define AVG_H

#include <stdint.h>
#include <stdbool.h>

#define AVG_MAX_WINDOW 32

typedef struct {
    uint16_t buffer[AVG_MAX_WINDOW];
    uint32_t sum;
    uint8_t window;
    uint8_t count;
    uint8_t head;
} avg_t;

bool avg_init(avg_t *a, uint8_t window);
uint16_t avg_push(avg_t *a, uint16_t value);
uint16_t avg_get(const avg_t *a);
uint8_t avg_count(const avg_t *a);
uint8_t avg_window(const avg_t *a);

#endif
