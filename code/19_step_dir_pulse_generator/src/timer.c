#include "timer.h"
#include <avr/io.h>

void timer3_init(uint32_t number){
    TCCR3A = 0;
    TCCR3B = (1 << WGM32);

    TCNT3 = 0;
    OCR3A = number;
    TIMSK3 = (1 << OCIE3A);  // 打开Timer3比较A中断的分开关
    // 最后启动Timer3：64分频
    TCCR3B |= (1 << CS31) | (1 << CS30);
}

void timer1_init(uint32_t number){
     // ............timer 1............
    TCCR1A = (1 << COM1A0);
    TCCR1B = (1 << WGM12);
    TCNT1 = 0;
    OCR1A = number;
    // 最后启动Timer1：64分频
    TCCR1B |= (1 << CS11) | (1 << CS10);
}
