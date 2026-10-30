#include <avr/io.h>
#include <util/delay.h>
#include <stdint.h>

void wait_ms(uint16_t duration_ms)
{
    for (uint16_t i = 0; i < duration_ms; i++)
    {
        _delay_ms(1);
    }
}

void blink(uint8_t times, uint16_t interval_ms)
{
    for (uint8_t i = 0; i < times; i++)
    {
        PORTB |= (1 << PORTB4);
        wait_ms(interval_ms);

        PORTB &= ~(1 << PORTB4);
        wait_ms(interval_ms);
    }
}

int main(void)
{
    // PB4：Arduino D10，外接LED
    DDRB |= (1 << DDB4);

    const uint8_t fast_times = 3;
    const uint16_t fast_interval_ms = 150;

    const uint16_t pause_ms = 1000;

    const uint8_t slow_times = 2;
    const uint16_t slow_interval_ms = 600;

    // 快速闪烁3次
    blink(fast_times, fast_interval_ms);

    // 熄灭停顿1秒
    wait_ms(pause_ms);

    // 慢速闪烁2次
    blink(slow_times, slow_interval_ms);

    // 最终保持熄灭
    while (1)
    {
    }
}