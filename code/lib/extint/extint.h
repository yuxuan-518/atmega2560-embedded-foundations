#ifndef EXTINT_H
#define EXTINT_H

#include <stdint.h>
#include <stdbool.h>

typedef enum {
    EXTINT_FALLING = 2,
    EXTINT_RISING = 3
} extint_edge_t;

void extint5_init(extint_edge_t edge, bool pullup);
void extint5_set_edge(extint_edge_t edge);
extint_edge_t extint5_get_edge(void);
void extint5_enable(void);
void extint5_disable(void);
bool extint5_is_enabled(void);
bool extint5_pin_high(void);
uint32_t extint5_snapshot(void);
uint32_t extint5_take(void);
void extint5_set_count(uint32_t value);

#endif
