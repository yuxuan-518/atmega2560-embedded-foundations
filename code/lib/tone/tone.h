#ifndef TONE_H
#define TONE_H

#include <stdint.h>
#include <stdbool.h>

#define TONE5_MAX_HZ (F_CPU / 2UL)
#define TONE5_DIVIDER_COUNT 4

bool tone5a_calc(uint32_t Hz, uint16_t *ocr, uint16_t *divider, uint32_t *actual_centihz);
void tone5a_init(bool idle_high);
bool tone5a_play(uint32_t Hz);
void tone5a_stop(void);
bool tone5a_is_running(void);
uint32_t tone5a_get_hz(void);
uint16_t tone5a_get_ocr(void);
uint16_t tone5a_get_divider(void);
uint32_t tone5a_get_actual_centihz(void);

#endif
