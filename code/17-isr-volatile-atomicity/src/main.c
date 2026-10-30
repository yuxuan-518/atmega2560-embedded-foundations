#include <avr/io.h>
#include <stdbool.h>
#include <util/delay.h>
#include <avr/interrupt.h>
#include <util/atomic.h>
#include "usart0.h"
#include <math.h>



// #define TIMER1_COMPA_vect_num 17
// #define TIMER1_COMPA_vect     _VECTOR(17)

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
    uint32_t last_time = 0;

    // ............打开port 6............
    DDRB |= (1 << DDB6);
    PORTB &= ~(1 << PORTB6);

    // ............usart............
    usart0_init();

    while(1){
        ATOMIC_BLOCK(ATOMIC_RESTORESTATE){
            now = system_ms;
        }

        int16_t highdigit = check_highdigit(now);
        int time[100] = {};
        uint16_t index_time = 0;
        for(;highdigit >= 0; highdigit--){

            int32_t temp_highdigit = highdigit;
            uint32_t temp = 1;
            for(;temp_highdigit >= 1;temp_highdigit--){
                temp = temp * 10;
            }


            uint8_t digit_time = (now / temp) % 10;
            time[index_time] = digit_time + '0';
            index_time++;
        }

        char array_upload[16] = "SYSTEM ALIVE";
        uint8_t index_send = 0;

        if(now - last_time >= 1000){
            last_time = now;
            PORTB ^= (1 << PORTB6);
            while(array_upload[index_send] != '\0'){
                usart0_tx_send(array_upload[index_send]);
                index_send++;
            }

            usart0_tx_send('\r');
            usart0_tx_send('\n');

            index_time = 0;
            while(time[index_time] != '\0'){
                usart0_tx_send(time[index_time]);
                index_time++;
            }

            usart0_tx_send('\r');
            usart0_tx_send('\n');
        }

        



    }
}