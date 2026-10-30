#include "pwm.h"
#include <avr/io.h>
#include <util/atomic.h>

static uint16_t pwm4_top = 0;
static uint8_t pwm4_duty = 0;
static uint32_t pwm4_hz = 0;

bool pwm4a_init(uint32_t Hz)
{
    if(Hz < PWM4_MIN_HZ || Hz > PWM4_MAX_HZ){
        return false;
    }

    uint32_t ticks = F_CPU / (PWM4_PRESCALER * Hz);

    if(ticks < 256 || ticks > 65536UL){
        return false;
    }

    PORTH &= ~(1 << PORTH3);
    DDRH |= (1 << DDH3);

    ATOMIC_BLOCK(ATOMIC_RESTORESTATE){
        TCCR4B = 0;
        TCCR4A = (1 << WGM41);
        TCCR4C = 0;
        ICR4 = ticks - 1;
        OCR4A = 0;
        TCNT4 = 0;
        TCCR4B = (1 << WGM43) | (1 << WGM42) | (1 << CS41);
    }

    pwm4_top = ticks - 1;
    pwm4_hz = Hz;
    pwm4a_set_duty(0);
    return true;
}

void pwm4a_set_duty(uint8_t duty)
{
    pwm4_duty = duty;

    if(duty == 0){
        ATOMIC_BLOCK(ATOMIC_RESTORESTATE){
            TCCR4A &= ~((1 << COM4A1) | (1 << COM4A0));
            PORTH &= ~(1 << PORTH3);
        }
        return;
    }

    if(duty == 255){
        ATOMIC_BLOCK(ATOMIC_RESTORESTATE){
            TCCR4A &= ~((1 << COM4A1) | (1 << COM4A0));
            PORTH |= (1 << PORTH3);
        }
        return;
    }

    uint16_t ocr = (uint16_t)(((uint32_t)duty * (pwm4_top + 1UL)) / 255UL - 1);

    ATOMIC_BLOCK(ATOMIC_RESTORESTATE){
        OCR4A = ocr;
        TCCR4A |= (1 << COM4A1);
    }
}

uint8_t pwm4a_get_duty(void)
{
    return pwm4_duty;
}

uint32_t pwm4a_get_hz(void)
{
    return pwm4_hz;
}

uint16_t pwm4a_get_top(void)
{
    return pwm4_top;
}
