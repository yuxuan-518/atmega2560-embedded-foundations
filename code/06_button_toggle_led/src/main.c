#include <avr/io.h>
#include <stdbool.h>
#include <util/delay.h>

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

    bool last_state = true;//1

    while (1)
    {
        bool button_State = (PINB & (1 << PINB4)) != 0;//0代表按下了，1代表没按下去

        if (button_State != last_state){
            if(button_State == 0){
                if(PORTB & (1 << PORTB5)){
                    PORTB &= ~(1 << PORTB5);
                } else {
                    PORTB |= (1 << PORTB5);
                }
            } 
            // _delay_ms(50);
        }
        last_state = button_State;
    }
}