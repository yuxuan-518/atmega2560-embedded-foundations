#include <avr/io.h>
#include <stdbool.h>
#include <util/delay.h>
#include <avr/interrupt.h>
#include <util/atomic.h>
#include <math.h>
#include "usart.h"

volatile uint32_t system_ms = 0;
ISR(TIMER1_COMPA_vect)
{
    system_ms++;
}

int main(void){
    // ............timer............
    TCCR1A = 0;
    TCCR1B = (1 << WGM12)|(1 << CS11) | (1 << CS10);
    TCNT1 = 0;
    OCR1A = 249;
    TIMSK1 = (1 << OCIE1A);  // 打开Timer1比较A中断的分开关
    sei();                    // 打开全局中断总开关
    // 隔离 防止复制 s 数的时候被吞掉另一节
    uint32_t now = 0;

    // ............LED............
    DDRB |= (1 << DDB6);
    PORTB &= ~(1 << PORTB6);
    uint32_t last_led = 0;

    // ............usart............
    usart0_init();
    uint32_t last_serial = 0;

    // ............button............
    DDRB &= ~(1 << DDB7);
    PORTB |= (1 << PORTB7);
    uint8_t debounce = 50;
    uint32_t last_button = 0;
    bool last_button_state = 1;
    bool confirmed_button_state = 1;
    uint32_t button_count = 0;

    while(1){
        ATOMIC_BLOCK(ATOMIC_RESTORESTATE){
            now = system_ms;
        }
        // LED
        if(now - last_led >= 500){
            last_led = now;
            PORTB ^= (1 << PORTB6);
        }

        bool LED_bool = PINB & (1 << PINB6);
        // button (1 代表没有按下, 0 代表按下了)
        bool current_button_state = (PINB & (1 << PINB7)) != 0;
        if(current_button_state != last_button_state){
            last_button_state = current_button_state;
            last_button = now;
        } else {
            if(current_button_state != confirmed_button_state){
                if (now - last_button >= debounce){
                    confirmed_button_state = current_button_state;

                    if(confirmed_button_state == false){
                        button_count++;
                    }
                }
            }
        }

        // Serial port regular output
        // TIME: 2000 ms, LED: OFF, BUTTON: 1
        if(now - last_serial >= 2000){
            last_serial = now;

            usart0_send_string("TIME:");
            usart0_send_uint32(now);
            usart0_send_string(" ms, LED: ");
            if(LED_bool == 1){
                usart0_send_string("ON, ");
            } else {
                usart0_send_string("OFF, ");
            }
            usart0_send_string("BUTTON: ");
            usart0_send_uint32(button_count);
            usart0_send_newline();
        }
    }
}