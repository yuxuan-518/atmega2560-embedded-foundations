#ifndef POT_H
#define POT_H

#include <stdint.h>
#include <stdbool.h>

bool pot_init(uint32_t now);
void pot_update(uint32_t now);

bool pot_set_source(uint8_t source);
uint8_t pot_get_source(void);
bool pot_source_valid(void);
uint8_t pot_source_duty(void);

bool pot_is_valid(uint8_t source);
uint16_t pot_get_raw(uint8_t source);
uint8_t pot_raw_to_duty(uint16_t raw);
uint32_t pot_raw_to_mv(uint16_t raw);
void pot_send_status(uint32_t now);

#endif
