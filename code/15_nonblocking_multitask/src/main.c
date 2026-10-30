#include <avr/io.h>
#include <stdbool.h>
#include <util/delay.h>

void usart0_init(){
    UCSR0B = (1 << RXEN0)|(1 << TXEN0);
    UCSR0C = (1 << UCSZ01) | (1 << UCSZ00);

    UBRR0H = 0;
    UBRR0L = 103;
}

uint8_t usart0_rx_read(){
    while(!(UCSR0A & (1 << RXC0))){
    }
    return UDR0;
}

void usart0_tx_send(uint8_t data){
    while(!(UCSR0A & (1 << UDRE0))){
    }
    UDR0 = data;
}

bool too_long_status = false;

//   计时函数
uint16_t time = 0;
void account_time(){
    time++;
    _delay_ms(1);
}

int main(void){
    usart0_init();

    DDRB &= ~(1 << DDB7); // button
    PORTB |= (1 << PORTB7);
    DDRB |= (1 << DDB6); // led

    bool last_state = 1;
    bool confirmed_state = 1;
    uint8_t debounce = 50;

    uint16_t button_time1 = 0;
    uint16_t button_time2 = 0;

    uint16_t last_led_time = 0;

    while(1){
        //_________________________________ 计时器 _________________________________
        account_time();
        //_________________________________ led _________________________________
        bool led_state = PINB & (1 << PINB6);
        if((uint16_t)(time - last_led_time) >= 1000){
            last_led_time = time;

            if(led_state){
                PORTB &= ~(1 << PORTB6);
            } else {
                PORTB |= (1 << PORTB6);
            }
        }
        //_________________________________ button _________________________________
        // 1 是 高电平 代表 按钮没被按下
        // 0 是 低电平 代表 按钮被按下
        bool current_state = PINB & (1 << PINB7);       // 读取

        if(current_state != last_state){
            button_time1 = time;
            last_state = current_state;
        } else if (current_state == last_state){
            button_time2 = time;

            if(button_time2 - button_time1 >= debounce && confirmed_state != current_state){
                confirmed_state = current_state;

                char state[] = "changed";
                uint8_t index_state = 0;
                while(state[index_state] != '\0'){
                    usart0_tx_send(state[index_state]);
                    index_state++;
                }
                usart0_tx_send('\r');
                usart0_tx_send('\n');
            }
        }
    }
}