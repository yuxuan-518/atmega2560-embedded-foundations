#include "ultrasonic.h"
#include "ustimer.h"

#include <avr/io.h>
#include <avr/interrupt.h>
#include <util/atomic.h>

static volatile bool echo_armed = false;
static volatile bool echo_rise_seen = false;
static volatile bool echo_fall_seen = false;
static volatile uint32_t echo_rise_us = 0;
static volatile uint32_t echo_fall_us = 0;
static volatile uint32_t echo_stray = 0;

ISR(PCINT0_vect)
{
    uint32_t now = ustimer_now_isr();
    bool high = (PINB & (1 << PINB4)) != 0;

    if(!echo_armed){
        echo_stray++;
        return;
    }

    if(!echo_rise_seen){
        echo_rise_us = now;
        echo_rise_seen = true;

        if(!high){
            echo_fall_us = now;
            echo_fall_seen = true;
            echo_armed = false;
        }
    } else if(!high){
        echo_fall_us = now;
        echo_fall_seen = true;
        echo_armed = false;
    }
}

void ultrasonic_init(void)
{
    ATOMIC_BLOCK(ATOMIC_RESTORESTATE){
        PORTH &= ~(1 << PORTH4);
        DDRH |= (1 << DDH4);

        DDRB &= ~(1 << DDB4);
        PORTB &= ~(1 << PORTB4);

        echo_armed = false;
        echo_rise_seen = false;
        echo_fall_seen = false;
        echo_stray = 0;

        PCMSK0 |= (1 << PCINT4);
        PCIFR = (1 << PCIF0);
        PCICR |= (1 << PCIE0);
    }
}

void ultrasonic_trig(bool high)
{
    ATOMIC_BLOCK(ATOMIC_RESTORESTATE){
        if(high){
            PORTH |= (1 << PORTH4);
        } else {
            PORTH &= ~(1 << PORTH4);
        }
    }
}

bool ultrasonic_echo_high(void)
{
    return (PINB & (1 << PINB4)) != 0;
}

void ultrasonic_arm(void)
{
    ATOMIC_BLOCK(ATOMIC_RESTORESTATE){
        PCIFR = (1 << PCIF0);
        echo_rise_seen = false;
        echo_fall_seen = false;
        echo_armed = true;
    }
}

void ultrasonic_disarm(void)
{
    ATOMIC_BLOCK(ATOMIC_RESTORESTATE){
        echo_armed = false;
        PCIFR = (1 << PCIF0);
    }
}

void ultrasonic_get_echo(ultrasonic_echo_t *echo)
{
    ATOMIC_BLOCK(ATOMIC_RESTORESTATE){
        echo->rise_seen = echo_rise_seen;
        echo->fall_seen = echo_fall_seen;
        echo->rise_us = echo_rise_us;
        echo->fall_us = echo_fall_us;
    }
}

uint32_t ultrasonic_get_stray(void)
{
    uint32_t value = 0;

    ATOMIC_BLOCK(ATOMIC_RESTORESTATE){
        value = echo_stray;
    }
    return value;
}
