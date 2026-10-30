#include <avr/io.h>
#include <stdbool.h>
#include <util/delay.h>
#include <avr/interrupt.h>
#include <util/atomic.h>
#include <math.h>
#include "usart.h"
#include "timer.h"
#include "motor.h"

volatile uint32_t system_ms = 0;
ISR(TIMER3_COMPA_vect)
{
    system_ms++;
}

int main(void){
    // ............ step 5/ dir 6............
    PORTB &= ~((1 << PORTB5) | (1 << PORTB6));
    DDRB |= (1 << DDB5) | (1 << DDB6);

    // timer init
    timer1_init(624);
    timer3_init(249);

    // 隔离 防止复制 s 数的时候被吞掉另一节
    uint32_t now = 0;

    sei();      // 打开全局中断总开关

    // ............usart............
    usart0_init();
    uint32_t last_serial = 0;

    // ............step............
    // uint32_t last_step = 0;
    // ............dir............
    uint32_t last_dir = 0;

    while(1){
        ATOMIC_BLOCK(ATOMIC_RESTORESTATE){
            now = system_ms;
        }

        if(now - last_serial >= 4000){
            last_serial = now;
            usart0_send_string("SYSTEM RUNNING ");
            usart0_send_uint32(now / 1000);
            usart0_send_string("ms");
            usart0_send_newline();
        }

        if(now - last_dir >= 5000){
            last_dir = now;
            motor_change_direction();
        }
    }
}