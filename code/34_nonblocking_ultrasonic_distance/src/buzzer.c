#include "buzzer.h"
#include "config.h"
#include "tone.h"
#include "usart.h"

#include <avr/pgmspace.h>

typedef struct {
    uint16_t hz;
    uint16_t ms;
} buzzer_note_t;

static const buzzer_note_t scale_notes[] PROGMEM = {
    {262, 250},
    {294, 250},
    {330, 250},
    {349, 250},
    {392, 250},
    {440, 250},
    {494, 250},
    {523, 500}
};

static const buzzer_note_t melody_notes[] PROGMEM = {
    {262, 300},
    {262, 300},
    {392, 300},
    {392, 300},
    {440, 300},
    {440, 300},
    {392, 600},
    {0, 200},
    {349, 300},
    {349, 300},
    {330, 300},
    {330, 300},
    {294, 300},
    {294, 300},
    {262, 600}
};

#define SCALE_COUNT (sizeof(scale_notes) / sizeof(scale_notes[0]))
#define MELODY_COUNT (sizeof(melody_notes) / sizeof(melody_notes[0]))

static buzzer_mode_t mode = BUZZER_IDLE;
static const buzzer_note_t *notes = 0;
static uint8_t note_count = 0;
static uint8_t note_index = 0;
static bool in_gap = false;
static uint16_t note_hz = 0;
static uint16_t note_ms = 0;
static uint32_t phase_start = 0;
static uint32_t phase_ms = 0;
static uint32_t resyncs = 0;

static void buzzer_read(uint8_t index)
{
    note_hz = pgm_read_word(&notes[index].hz);
    note_ms = pgm_read_word(&notes[index].ms);
}

static bool buzzer_has_gap(void)
{
    return note_hz != 0 && note_ms > CONFIG_BUZZER_GAP_MS;
}

static void buzzer_start_phase(uint32_t start)
{
    phase_start = start;

    if(in_gap){
        phase_ms = CONFIG_BUZZER_GAP_MS;
        tone5a_stop();
        return;
    }

    buzzer_read(note_index);

    if(buzzer_has_gap()){
        phase_ms = note_ms - CONFIG_BUZZER_GAP_MS;
    } else {
        phase_ms = note_ms;
    }

    tone5a_play(note_hz);
}

static void buzzer_start_table(buzzer_mode_t new_mode, const buzzer_note_t *table, uint8_t count, uint32_t now)
{
    mode = new_mode;
    notes = table;
    note_count = count;
    note_index = 0;
    in_gap = false;
    buzzer_start_phase(now);
}

void buzzer_init(void)
{
    tone5a_init(CONFIG_BUZZER_IDLE_HIGH);
    mode = BUZZER_IDLE;
    notes = 0;
    note_count = 0;
    note_index = 0;
    in_gap = false;
    note_hz = 0;
    note_ms = 0;
    resyncs = 0;
}

void buzzer_stop(void)
{
    tone5a_stop();
    mode = BUZZER_IDLE;
    notes = 0;
    note_count = 0;
    note_index = 0;
    in_gap = false;
    note_hz = 0;
}

void buzzer_update(uint32_t now)
{
    if(mode == BUZZER_IDLE){
        return;
    }

    uint32_t elapsed = now - phase_start;

    if(elapsed < phase_ms){
        return;
    }

    uint32_t start = phase_start + phase_ms;

    if(elapsed - phase_ms > CONFIG_BUZZER_LATE_MS){
        start = now;
        resyncs++;
    }

    if(mode == BUZZER_TONE){
        buzzer_stop();
        return;
    }

    if(!in_gap && buzzer_has_gap()){
        in_gap = true;
    } else {
        in_gap = false;
        note_index++;

        if(note_index >= note_count){
            buzzer_stop();
            return;
        }
    }

    buzzer_start_phase(start);
}

bool buzzer_tone(uint32_t Hz, uint32_t ms, uint32_t now)
{
    if(ms < CONFIG_BUZZER_MIN_MS || ms > CONFIG_BUZZER_MAX_MS){
        return false;
    }

    if(Hz != 0 && (Hz < CONFIG_BUZZER_MIN_HZ || Hz > CONFIG_BUZZER_MAX_HZ)){
        return false;
    }

    if(!tone5a_play(Hz)){
        return false;
    }

    mode = BUZZER_TONE;
    notes = 0;
    note_count = 1;
    note_index = 0;
    in_gap = false;
    note_hz = Hz;
    note_ms = ms;
    phase_start = now;
    phase_ms = ms;
    return true;
}

void buzzer_play_scale(uint32_t now)
{
    buzzer_start_table(BUZZER_SCALE, scale_notes, SCALE_COUNT, now);
}

void buzzer_play_melody(uint32_t now)
{
    buzzer_start_table(BUZZER_MELODY, melody_notes, MELODY_COUNT, now);
}

buzzer_mode_t buzzer_get_mode(void)
{
    return mode;
}

uint8_t buzzer_get_note(void)
{
    return note_index;
}

uint8_t buzzer_get_note_count(void)
{
    return note_count;
}

bool buzzer_in_gap(void)
{
    return in_gap;
}

uint16_t buzzer_get_hz(void)
{
    if(mode == BUZZER_IDLE || in_gap){
        return 0;
    }
    return note_hz;
}

uint32_t buzzer_get_left(uint32_t now)
{
    if(mode == BUZZER_IDLE){
        return 0;
    }

    uint32_t elapsed = now - phase_start;

    if(elapsed >= phase_ms){
        return 0;
    }
    return phase_ms - elapsed;
}

uint32_t buzzer_get_total_left(uint32_t now)
{
    uint32_t left = buzzer_get_left(now);

    if(mode == BUZZER_IDLE || mode == BUZZER_TONE){
        return left;
    }

    if(!in_gap && buzzer_has_gap()){
        left += CONFIG_BUZZER_GAP_MS;
    }

    for(uint8_t i = note_index + 1; i < note_count; i++){
        left += pgm_read_word(&notes[i].ms);
    }
    return left;
}

uint32_t buzzer_get_resyncs(void)
{
    return resyncs;
}

uint8_t buzzer_get_table(buzzer_mode_t table_mode, uint8_t index, uint16_t *Hz, uint16_t *ms)
{
    const buzzer_note_t *table = 0;
    uint8_t count = 0;

    if(table_mode == BUZZER_SCALE){
        table = scale_notes;
        count = SCALE_COUNT;
    } else if(table_mode == BUZZER_MELODY){
        table = melody_notes;
        count = MELODY_COUNT;
    }

    if(table != 0 && index < count){
        *Hz = pgm_read_word(&table[index].hz);
        *ms = pgm_read_word(&table[index].ms);
    }
    return count;
}

static void buzzer_send_centihz(uint32_t centihz)
{
    usart0_send_uint32(centihz / 100);
    usart0_send_string_P(PSTR("."));

    if(centihz % 100 < 10){
        usart0_send_string_P(PSTR("0"));
    }
    usart0_send_uint32(centihz % 100);
}

void buzzer_send_status(uint32_t now)
{
    usart0_send_string_P(PSTR("BUZZER: "));

    if(mode == BUZZER_IDLE){
        usart0_send_string_P(PSTR("IDLE"));
    } else if(mode == BUZZER_TONE){
        usart0_send_string_P(PSTR("TONE"));
    } else if(mode == BUZZER_SCALE){
        usart0_send_string_P(PSTR("SCALE"));
    } else {
        usart0_send_string_P(PSTR("MELODY"));
    }

    if(mode != BUZZER_IDLE){
        usart0_send_string_P(PSTR(", NOTE "));
        usart0_send_uint32(note_index + 1);
        usart0_send_string_P(PSTR("/"));
        usart0_send_uint32(note_count);

        if(in_gap){
            usart0_send_string_P(PSTR(" GAP"));
        } else if(note_hz == 0){
            usart0_send_string_P(PSTR(" REST"));
        }
    }

    usart0_send_string_P(PSTR(", REQ "));
    usart0_send_uint32(buzzer_get_hz());
    usart0_send_string_P(PSTR(" Hz, ACT "));
    buzzer_send_centihz(tone5a_get_actual_centihz());
    usart0_send_string_P(PSTR(" Hz"));

    if(tone5a_is_running()){
        usart0_send_string_P(PSTR(" (OCR5A "));
        usart0_send_uint32(tone5a_get_ocr());
        usart0_send_string_P(PSTR(", DIV "));
        usart0_send_uint32(tone5a_get_divider());
        usart0_send_string_P(PSTR(")"));
    }

    usart0_send_string_P(PSTR(", LEFT "));
    usart0_send_uint32(buzzer_get_left(now));
    usart0_send_string_P(PSTR(" ms, TOTAL "));
    usart0_send_uint32(buzzer_get_total_left(now));
    usart0_send_string_P(PSTR(" ms, RESYNC "));
    usart0_send_uint32(resyncs);
    usart0_send_newline();
}
