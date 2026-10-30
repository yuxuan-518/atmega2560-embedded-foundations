#include "tone.h"
#include <avr/io.h>
#include <util/atomic.h>

static const uint16_t tone5_dividers[TONE5_DIVIDER_COUNT] = {1, 8, 64, 256};
static const uint8_t tone5_clocks[TONE5_DIVIDER_COUNT] = {
    (1 << CS50),
    (1 << CS51),
    (1 << CS51) | (1 << CS50),
    (1 << CS52)
};

static bool tone5_idle_high = false;
static bool tone5_running = false;
static uint32_t tone5_hz = 0;
static uint16_t tone5_ocr = 0;
static uint8_t tone5_index = 0;

static bool tone5a_find(uint32_t Hz, uint16_t *ocr, uint8_t *index)
{
    if(Hz == 0 || Hz > TONE5_MAX_HZ){
        return false;
    }

    for(uint8_t i = 0; i < TONE5_DIVIDER_COUNT; i++){
        uint32_t step = 2UL * tone5_dividers[i] * Hz;
        uint32_t ticks = (F_CPU + step / 2) / step;

        if(ticks >= 1 && ticks <= 65536UL){
            *ocr = ticks - 1;
            *index = i;
            return true;
        }
    }

    return false;
}

static uint32_t tone5a_centihz(uint16_t ocr, uint8_t index)
{
    uint32_t step = 2UL * tone5_dividers[index] * (ocr + 1UL);
    return (F_CPU * 100UL + step / 2) / step;
}

static void tone5a_idle_pin(void)
{
    if(tone5_idle_high){
        PORTL |= (1 << PORTL3);
    } else {
        PORTL &= ~(1 << PORTL3);
    }
}

bool tone5a_calc(uint32_t Hz, uint16_t *ocr, uint16_t *divider, uint32_t *actual_centihz)
{
    uint16_t value = 0;
    uint8_t index = 0;

    if(!tone5a_find(Hz, &value, &index)){
        return false;
    }

    *ocr = value;
    *divider = tone5_dividers[index];
    *actual_centihz = tone5a_centihz(value, index);
    return true;
}

void tone5a_init(bool idle_high)
{
    tone5_idle_high = idle_high;

    ATOMIC_BLOCK(ATOMIC_RESTORESTATE){
        TCCR5B = 0;
        TCCR5A = 0;
        TCCR5C = 0;
        TIMSK5 = 0;
        TCNT5 = 0;
        OCR5A = 0;
    }

    tone5a_idle_pin();
    DDRL |= (1 << DDL3);

    tone5_running = false;
    tone5_hz = 0;
    tone5_ocr = 0;
    tone5_index = 0;
}

bool tone5a_play(uint32_t Hz)
{
    if(Hz == 0){
        tone5a_stop();
        return true;
    }

    uint16_t ocr = 0;
    uint8_t index = 0;

    if(!tone5a_find(Hz, &ocr, &index)){
        return false;
    }

    if(tone5_running && ocr == tone5_ocr && index == tone5_index){
        tone5_hz = Hz;
        return true;
    }

    ATOMIC_BLOCK(ATOMIC_RESTORESTATE){
        TCCR5B = (1 << WGM52);
        TCNT5 = 0;
        OCR5A = ocr;

        if(!tone5_running){
            if(tone5_idle_high){
                TCCR5A = (1 << COM5A1) | (1 << COM5A0);
            } else {
                TCCR5A = (1 << COM5A1);
            }
            TCCR5C = (1 << FOC5A);
        }

        TCCR5A = (1 << COM5A0);
        TCCR5B = (1 << WGM52) | tone5_clocks[index];
    }

    tone5_running = true;
    tone5_hz = Hz;
    tone5_ocr = ocr;
    tone5_index = index;
    return true;
}

void tone5a_stop(void)
{
    ATOMIC_BLOCK(ATOMIC_RESTORESTATE){
        tone5a_idle_pin();
        TCCR5A = 0;
        TCCR5B = 0;
        TCNT5 = 0;
    }

    tone5_running = false;
    tone5_hz = 0;
}

bool tone5a_is_running(void)
{
    return tone5_running;
}

uint32_t tone5a_get_hz(void)
{
    return tone5_hz;
}

uint16_t tone5a_get_ocr(void)
{
    return tone5_ocr;
}

uint16_t tone5a_get_divider(void)
{
    return tone5_dividers[tone5_index];
}

uint32_t tone5a_get_actual_centihz(void)
{
    if(!tone5_running){
        return 0;
    }
    return tone5a_centihz(tone5_ocr, tone5_index);
}
