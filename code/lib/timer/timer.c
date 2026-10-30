#include "timer.h"
#include "usart.h"
#include <avr/io.h>
#include <avr/interrupt.h>
#include <util/atomic.h>

#define TIMER1_PRESCALER 8UL
#define TIMER3_PRESCALER 64UL

static volatile uint16_t timer1_next_ocr = 0;
static volatile uint32_t timer1_steps = 0;
static volatile uint8_t timer1_count_level = 0;
static volatile bool timer1_locked = false;
static volatile uint32_t timer1_budget = 0;
static volatile bool timer1_budget_on = false;

ISR(TIMER1_COMPA_vect)
{
    OCR1A = timer1_next_ocr;

    uint8_t level = (PINB & (1 << PINB5)) != 0;
    if(level == timer1_count_level){
        timer1_steps++;

        if(timer1_budget_on){
            if(timer1_budget > 0){
                timer1_budget--;
            }

            if(timer1_budget == 0){
                TCCR1B &= ~((1 << CS12) |
                            (1 << CS11) |
                            (1 << CS10));
                timer1_budget_on = false;
            }
        }
    }
}

static uint32_t timer1_hz_to_ocr(uint32_t Hz)
{
    if(Hz == 0){
        usart0_send_string("WARNING timer1 Hz == 0");
        usart0_send_newline();
        return 0;
    }

    uint32_t divider = 2UL * TIMER1_PRESCALER * Hz;
    uint32_t OCR_value = F_CPU / divider;

    if(OCR_value == 0 || OCR_value > 65536UL){
        usart0_send_string("WARNING timer1 Hz out of range");
        usart0_send_newline();
        return 0;
    }

    return OCR_value;
}

bool timer1_init(uint32_t Hz)
{
    TCCR1A = (1 << COM1A0);
    TCCR1B = (1 << WGM12);

    ATOMIC_BLOCK(ATOMIC_RESTORESTATE){
        TCNT1 = 0;
    }

    TIMSK1 = (1 << OCIE1A);

    return timer1_set_hz(Hz);
}

bool timer1_set_hz(uint32_t Hz)
{
    uint32_t OCR_value = timer1_hz_to_ocr(Hz);

    if(OCR_value == 0){
        return false;
    }

    ATOMIC_BLOCK(ATOMIC_RESTORESTATE){
        timer1_next_ocr = OCR_value - 1;
        OCR1A = OCR_value - 1;
    }

    return true;
}

bool timer1_update_hz(uint32_t Hz)
{
    uint32_t OCR_value = timer1_hz_to_ocr(Hz);

    if(OCR_value == 0){
        return false;
    }

    ATOMIC_BLOCK(ATOMIC_RESTORESTATE){
        timer1_next_ocr = OCR_value - 1;
    }

    return true;
}

bool timer1_is_running(void)
{
    uint8_t cs_bits = TCCR1B & ((1 << CS12) |
                                (1 << CS11) |
                                (1 << CS10));
    return cs_bits != 0;
}

bool timer1_start(void)
{
    bool started = false;

    ATOMIC_BLOCK(ATOMIC_RESTORESTATE){
        if(!timer1_locked){
            if(!timer1_is_running()){
                TCNT1 = 0;
                TCCR1B |= (1 << CS11);
            }
            started = true;
        }
    }

    return started;
}

void timer1_stop(void)
{
    ATOMIC_BLOCK(ATOMIC_RESTORESTATE){
        TCCR1B &= ~((1 << CS12) |
                    (1 << CS11) |
                    (1 << CS10));
        TCNT1 = 0;
    }
}

void timer1_lock(void)
{
    ATOMIC_BLOCK(ATOMIC_RESTORESTATE){
        timer1_locked = true;
        TCCR1B &= ~((1 << CS12) |
                    (1 << CS11) |
                    (1 << CS10));
        TCNT1 = 0;
    }
}

void timer1_unlock(void)
{
    timer1_locked = false;
}

bool timer1_is_locked(void)
{
    return timer1_locked;
}

void timer1_set_count_level(uint8_t level)
{
    timer1_count_level = (level != 0);
}

uint32_t timer1_take_steps(void)
{
    uint32_t steps = 0;

    ATOMIC_BLOCK(ATOMIC_RESTORESTATE){
        steps = timer1_steps;
        timer1_steps = 0;
    }

    return steps;
}

void timer1_set_budget(uint32_t steps)
{
    ATOMIC_BLOCK(ATOMIC_RESTORESTATE){
        timer1_budget = steps;
        timer1_budget_on = (steps != 0);
    }
}

uint32_t timer1_get_budget(void)
{
    uint32_t steps = 0;

    ATOMIC_BLOCK(ATOMIC_RESTORESTATE){
        steps = timer1_budget;
    }

    return steps;
}

bool timer3_init(uint32_t Hz)
{
    if(Hz == 0){
        usart0_send_string("WARNING timer3 Hz == 0");
        usart0_send_newline();
        return false;
    }

    uint32_t divider = TIMER3_PRESCALER * Hz;
    uint32_t OCR_value = F_CPU / divider;

    if(OCR_value == 0 || OCR_value > 65536UL){
        usart0_send_string("WARNING timer3 Hz out of range");
        usart0_send_newline();
        return false;
    }

    TCCR3A = 0;
    TCCR3B = (1 << WGM32);
    TCNT3 = 0;
    OCR3A = OCR_value - 1;
    TIMSK3 = (1 << OCIE3A);

    TCCR3B |= (1 << CS31) | (1 << CS30);
    return true;
}
