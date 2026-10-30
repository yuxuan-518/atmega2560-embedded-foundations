#include "motor.h"

#include <util/delay.h>
#include <avr/io.h>

void motor_change_direction(void)
{
    // 停止Timer1
    TCCR1B &= ~((1 << CS12) |
                (1 << CS11) |
                (1 << CS10));

    // 清除未完成的计数
    TCNT1 = 0;

    PORTB ^= (1 << PORTB5);
    _delay_us(10);

    // 改变DIR
    PORTB ^= (1 << PORTB6);

    // 让DIR稳定至少5 µs
    _delay_us(10);

    // 恢复Timer1，64分频
    TCCR1B |= (1 << CS11) | (1 << CS10);
}
