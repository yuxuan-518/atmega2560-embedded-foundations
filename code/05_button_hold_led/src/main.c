#include <avr/io.h>

int main(void)
{
    // PB4（D10）设置为输入：按钮
    DDRB &= ~(1 << DDB4);

    // PB5（D11）设置为输出：LED
    DDRB |= (1 << DDB5);

    // PB4输入模式下写1：启用内部上拉
    PORTB |= (1 << PORTB4);

    // LED初始状态：熄灭
    PORTB &= ~(1 << PORTB5);

    while (1)
    {
        // 内部上拉：按钮按下时PINB4为0
        if (!(PINB & (1 << PINB4)))
        {
            // 按下：点亮LED
            PORTB |= (1 << PORTB5);
        }
        else
        {
            // 松开：熄灭LED
            PORTB &= ~(1 << PORTB5);
        }
    }
}