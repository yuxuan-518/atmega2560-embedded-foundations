#include "extint.h"
#include <avr/io.h>
#include <avr/interrupt.h>
#include <util/atomic.h>

static volatile uint32_t extint5_count = 0;
static extint_edge_t extint5_edge = EXTINT_FALLING;

ISR(INT5_vect)
{
    extint5_count++;
}

static void extint5_write_edge(extint_edge_t edge)
{
    uint8_t bits = 0;

    if(edge == EXTINT_RISING){
        bits = (1 << ISC51) | (1 << ISC50);
    } else {
        bits = (1 << ISC51);
    }

    EICRB = (EICRB & ~((1 << ISC51) | (1 << ISC50))) | bits;
    extint5_edge = edge;
}

void extint5_init(extint_edge_t edge, bool pullup)
{
    ATOMIC_BLOCK(ATOMIC_RESTORESTATE){
        EIMSK &= ~(1 << INT5);
        DDRE &= ~(1 << DDE5);

        if(pullup){
            PORTE |= (1 << PORTE5);
        } else {
            PORTE &= ~(1 << PORTE5);
        }

        extint5_write_edge(edge);
        EIFR = (1 << INTF5);
        extint5_count = 0;
    }
}

void extint5_set_edge(extint_edge_t edge)
{
    ATOMIC_BLOCK(ATOMIC_RESTORESTATE){
        uint8_t enabled = EIMSK & (1 << INT5);

        EIMSK &= ~(1 << INT5);
        extint5_write_edge(edge);
        EIFR = (1 << INTF5);
        EIMSK |= enabled;
    }
}

extint_edge_t extint5_get_edge(void)
{
    return extint5_edge;
}

void extint5_enable(void)
{
    ATOMIC_BLOCK(ATOMIC_RESTORESTATE){
        EIFR = (1 << INTF5);
        EIMSK |= (1 << INT5);
    }
}

void extint5_disable(void)
{
    ATOMIC_BLOCK(ATOMIC_RESTORESTATE){
        EIMSK &= ~(1 << INT5);
    }
}

bool extint5_is_enabled(void)
{
    return (EIMSK & (1 << INT5)) != 0;
}

bool extint5_pin_high(void)
{
    return (PINE & (1 << PINE5)) != 0;
}

uint32_t extint5_snapshot(void)
{
    uint32_t value = 0;

    ATOMIC_BLOCK(ATOMIC_RESTORESTATE){
        value = extint5_count;
    }
    return value;
}

uint32_t extint5_take(void)
{
    uint32_t value = 0;

    ATOMIC_BLOCK(ATOMIC_RESTORESTATE){
        value = extint5_count;
        extint5_count = 0;
    }
    return value;
}

void extint5_set_count(uint32_t value)
{
    ATOMIC_BLOCK(ATOMIC_RESTORESTATE){
        extint5_count = value;
    }
}
