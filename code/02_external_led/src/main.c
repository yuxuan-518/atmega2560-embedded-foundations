#include <avr/io.h>
#include <util/delay.h>

int main(void)
{
    // PB7：Arduino D13，板载L灯
    DDRB |= (1 << DDB7);

    // PB4：Arduino D10，外接黄色LED
    DDRB |= (1 << DDB4);

    while (1)
    {
        // 板载L灯亮200 ms
        PORTB |= (1 << PORTB7);
        _delay_ms(200);

        // 板载L灯灭200 ms
        PORTB &= ~(1 << PORTB7);
        _delay_ms(200);

        // 外接黄色LED亮200 ms
        PORTB |= (1 << PORTB4);
        _delay_ms(200);

        // 外接黄色LED灭200 ms
        PORTB &= ~(1 << PORTB4);
        _delay_ms(200);
    }
}