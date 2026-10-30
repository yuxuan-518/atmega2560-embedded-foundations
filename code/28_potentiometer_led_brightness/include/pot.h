#ifndef POT_H
#define POT_H

#include <stdint.h>
#include <stdbool.h>

void pot_init(uint32_t now);
void pot_update(uint32_t now);

bool pot_is_valid(void);
uint16_t pot_get_raw(void);
uint32_t pot_get_mv(void);
uint8_t pot_raw_to_duty(uint16_t raw);
uint32_t pot_raw_to_mv(uint16_t raw);
void pot_send_status(uint32_t now);

#endif
