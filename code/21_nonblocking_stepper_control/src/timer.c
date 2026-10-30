#include "timer.h"
#include <avr/io.h>
#include "usart.h"

#define TIMER_PRESCALER 64UL

bool timer1_set_hz(uint32_t Hz)
{
    if(Hz == 0){
        usart0_send_string("WARNING timer1 Hz == 0");
        usart0_send_newline();
        return false;
    }

    uint32_t divider = 2UL * TIMER_PRESCALER * Hz;
    uint32_t OCR_value = F_CPU / divider;

    if(OCR_value == 0 || OCR_value > 65536UL){
        usart0_send_string("WARNING timer1 Hz out of range");
        usart0_send_newline();
        return false;
    }

    OCR1A = OCR_value - 1;
    return true;
}

bool timer1_init(uint32_t Hz)
{
    // ............timer 1............
    TCCR1A = (1 << COM1A0);
    TCCR1B = (1 << WGM12);
    TCNT1 = 0;

    return timer1_set_hz(Hz);
}

bool timer3_init(uint32_t Hz)
{
    if(Hz == 0){
        usart0_send_string("WARNING timer3 Hz == 0");
        usart0_send_newline();
        return false;
    }

    uint32_t divider = TIMER_PRESCALER * Hz;
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
    TIMSK3 = (1 << OCIE3A);  // 打开Timer3比较A中断的分开关

    // 最后启动Timer3：64分频
    TCCR3B |= (1 << CS31) | (1 << CS30);
    return true;
}
