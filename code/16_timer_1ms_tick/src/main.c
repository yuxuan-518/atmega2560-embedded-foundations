#include <avr/io.h>
#include <stdbool.h>
#include <util/delay.h>
#include <avr/interrupt.h>
#include <util/atomic.h>

// #define TIMER1_COMPA_vect_num 17
// #define TIMER1_COMPA_vect     _VECTOR(17)

volatile uint16_t system_ms = 0;
uint16_t mode = 0;
ISR(TIMER1_COMPA_vect)
{
    system_ms++;

}

int main(void){
    TCCR1A = 0;
    TCCR1B = (1 << WGM12)|(1 << CS11) | (1 << CS10);
    TCNT1 = 0;
    OCR1A = 249;

    TIMSK1 = (1 << OCIE1A);  // 打开Timer1比较A中断的分开关
    sei();                    // 打开全局中断总开关

    DDRB |= (1 << DDB6);
    PORTB &= ~(1 << PORTB6);

    uint16_t last_mode_time = 0;
    uint16_t now = 0;

    while(1){
        ATOMIC_BLOCK(ATOMIC_RESTORESTATE){
            now = system_ms;
        }

        if(now - last_mode_time >= 1000){
            last_mode_time = now;
            mode++;

            if(mode == 3){
                mode = 1;
            }
        }

        if(mode == 1){
            PORTB |= (1 << PORTB6);
        } else if (mode == 2){
            PORTB &= ~(1 << PORTB6);
        }
    }
}