#include "estop.h"
#include "timer.h"

#include <avr/io.h>
#include <avr/interrupt.h>
#include <util/delay.h>

static volatile bool estop_latched = false;

ISR(INT4_vect)
{
    timer1_lock();
    estop_latched = true;
}

void estop_init(void)
{
    DDRE &= ~(1 << DDE4);
    PORTE |= (1 << PORTE4);
    _delay_us(50);

#if ESTOP_ACTIVE_LEVEL == 0
    EICRB = (EICRB & ~((1 << ISC41) | (1 << ISC40))) | (1 << ISC41);
#else
    EICRB = EICRB | (1 << ISC41) | (1 << ISC40);
#endif

    EIFR = (1 << INTF4);
    EIMSK |= (1 << INT4);
}

bool estop_input_active(void)
{
    uint8_t level = (PINE & (1 << PINE4)) != 0;
    return level == ESTOP_ACTIVE_LEVEL;
}

bool estop_is_latched(void)
{
    return estop_latched;
}

void estop_latch(void)
{
    timer1_lock();
    estop_latched = true;
}

void estop_clear(void)
{
    estop_latched = false;
}
