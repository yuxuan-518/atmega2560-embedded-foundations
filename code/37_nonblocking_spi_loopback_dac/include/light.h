#ifndef LIGHT_H
#define LIGHT_H

#include <stdint.h>
#include <stdbool.h>

typedef enum {
    LIGHT_NO_DATA,
    LIGHT_STALE,
    LIGHT_OK
} light_data_t;

void light_init(void);
void light_update(uint32_t now);

bool light_set_thresholds(uint16_t on_level, uint16_t off_level);
bool light_set_window(uint8_t window);
void light_set_invert(bool invert);

light_data_t light_get_data(void);
bool light_lamp_on(void);
uint16_t light_get_level(void);
void light_send_status(uint32_t now);

#endif
