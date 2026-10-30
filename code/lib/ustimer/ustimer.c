#include "ustimer.h"
#include <avr/io.h>
#include <avr/interrupt.h>
#include <util/atomic.h>

static volatile uint32_t ustimer_overflows = 0;

ISR(TIMER2_OVF_vect)
{
    ustimer_overflows++;
}

void ustimer_init(void)
{
    ATOMIC_BLOCK(ATOMIC_RESTORESTATE){
        TIMSK2 = 0;
        TCCR2B = 0;
        ASSR &= ~((1 << EXCLK) | (1 << AS2));
        TCCR2A = 0;
        TCNT2 = 0;
        TIFR2 = (1 << TOV2) | (1 << OCF2A) | (1 << OCF2B);
        ustimer_overflows = 0;
        TIMSK2 = (1 << TOIE2);
        TCCR2B = (1 << CS22);
    }
}

uint32_t ustimer_now_isr(void)
{
    uint8_t count = TCNT2;
    uint32_t overflows = ustimer_overflows;

    if((TIFR2 & (1 << TOV2)) && count < 255){
        overflows++;
    }

    return ((overflows << 8) | count) * USTIMER_US_PER_TICK;
}

uint32_t ustimer_now(void)
{
    uint32_t value = 0;

    ATOMIC_BLOCK(ATOMIC_RESTORESTATE){
        value = ustimer_now_isr();
    }
    return value;
}
