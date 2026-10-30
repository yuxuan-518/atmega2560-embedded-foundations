#include <avr/io.h>
#include <util/delay.h>

int main(void)
{
    // PB7配置为输出，对应Arduino D13和板载L灯
    DDRB |= (1 << DDB7);

    while (1)
    {
        // PB7输出HIGH，点亮板载L灯
        PORTB |= (1 << PORTB7);
        _delay_ms(500);

        // PB7输出LOW，熄灭板载L灯
        PORTB &= ~(1 << PORTB7);
        _delay_ms(500);
    }
}