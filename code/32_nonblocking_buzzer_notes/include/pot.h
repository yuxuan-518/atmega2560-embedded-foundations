#ifndef POT_H
#define POT_H

#include <stdint.h>
#include <stdbool.h>

typedef struct {
    uint16_t raw;
    uint16_t avg;
    uint16_t cal;
    uint8_t map;
    uint8_t hold;
    uint8_t count;
    uint8_t window;
    uint8_t hyst;
    uint16_t cal_low;
    uint16_t cal_high;
    bool ready;
} pot_view_t;

void pot_init(void);
void pot_update(void);

bool pot_set_source(uint8_t source);
uint8_t pot_get_source(void);
bool pot_source_valid(void);
uint8_t pot_source_duty(void);

bool pot_set_window(uint8_t channel, uint8_t window);
bool pot_set_hyst(uint8_t channel, uint8_t threshold);
bool pot_set_cal(uint8_t channel, uint16_t low, uint16_t high);
void pot_set_default(uint8_t channel);
bool pot_get_view(uint8_t channel, pot_view_t *view);

bool pot_is_valid(uint8_t source);
uint16_t pot_get_raw(uint8_t source);
uint8_t pot_raw_to_duty(uint16_t raw);
uint32_t pot_raw_to_mv(uint16_t raw);
void pot_send_filter_status(uint8_t config_channel);

#endif
