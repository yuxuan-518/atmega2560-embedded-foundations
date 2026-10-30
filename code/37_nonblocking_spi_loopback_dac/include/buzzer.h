#ifndef BUZZER_H
#define BUZZER_H

#include <stdint.h>
#include <stdbool.h>

typedef enum {
    BUZZER_IDLE,
    BUZZER_TONE,
    BUZZER_SCALE,
    BUZZER_MELODY
} buzzer_mode_t;

void buzzer_init(void);
void buzzer_update(uint32_t now);

bool buzzer_tone(uint32_t Hz, uint32_t ms, uint32_t now);
void buzzer_play_scale(uint32_t now);
void buzzer_play_melody(uint32_t now);
void buzzer_stop(void);

buzzer_mode_t buzzer_get_mode(void);
uint8_t buzzer_get_note(void);
uint8_t buzzer_get_note_count(void);
bool buzzer_in_gap(void);
uint16_t buzzer_get_hz(void);
uint32_t buzzer_get_left(uint32_t now);
uint32_t buzzer_get_total_left(uint32_t now);
uint32_t buzzer_get_resyncs(void);
uint8_t buzzer_get_table(buzzer_mode_t table_mode, uint8_t index, uint16_t *Hz, uint16_t *ms);
void buzzer_send_status(uint32_t now);

#endif
