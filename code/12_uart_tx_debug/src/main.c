#include <avr/io.h>
#include <stdbool.h>
#include <util/delay.h>
#include <math.h>

void blink_slow(uint8_t time, uint8_t port){
    for(int i = 0; i < time; i++){
        PORTB |= (1 << port);
        _delay_ms(150);
        PORTB &= ~(1 << port);
        _delay_ms(150);
    }
}

void blink_fast(uint8_t time, uint8_t port){
    for(int i = 0; i < time; i++){
        PORTB |= (1 << port);
        _delay_ms(50);
        PORTB &= ~(1 << port);
        _delay_ms(50);
    }
}

void usart0_init(void){
    UBRR0H = 0;
    UBRR0L = 0b01100111;        // 直接写103比较好

    UCSR0B = (1 << TXEN0);
    UCSR0C = (1 << UCSZ01) | (1 << UCSZ00);
}

void usart0_send_byte(uint8_t data){
    while(!(UCSR0A & (1 << UDRE0))){
    }
    UDR0 = data;
}

uint8_t check_highdigit(uint16_t number){
    uint8_t i = 0;
    while(number/pow(10,i) >= 10){
        i++;
    }

    return i;
}

int main(void){
    // button 1 input pull-up。   PB4 D10
    DDRB &= ~(1 << DDB4);
    PORTB |= (1 << PORTB4);

    // button 2 input pull-up.    PB5 D11
    DDRB &= ~(1 << DDB5);
    PORTB |= (1 << PORTB5);

    // LED OUTPUT NOTLIGHT.     PB6 D12
    DDRB |= (1 << DDB6);
    PORTB &= ~(1 << PORTB6);

    // LED OUTPUT NOTLIGHT.     PB7 D13
    DDRB |= (1 << DDB7);
    PORTB &= ~(1 << PORTB7);


    // true == 1,  false == 0
    // button 1 没按下 ； 0 按下
    
    //消抖系统变量：
    uint8_t debounce_delay = 50;

    // button 1.    PB4
    bool last_reading_1 = true;
    bool confirmed_state_1 = true;
    bool last_confirmed_state_1 = false;
    bool pressed_1 = false;
    uint8_t stable_count_1 = 0;
    uint8_t pressed_1_time = 0;

    // button 2.    PB
    bool last_reading = true;
    bool confirmed_state = true;
    bool last_confirmed_state = false;
    bool pressed = false;
    uint8_t stable_count = 0;
    uint8_t pressed_time = 0;
    uint16_t press_account_B = 0;
    uint16_t press_account_A = 0;
    int press_account_AA[100] = {};
    uint8_t index_A = 0;
    int press_account_BB[100] = {};
    uint8_t index_B = 0;
    
    usart0_init();

    char message_init[] = "BOOT\r\n";
    uint8_t i_message_init = 0;
    while(message_init[i_message_init] != '\0'){
        usart0_send_byte(message_init[i_message_init]);
        i_message_init ++;
    }

    char message_A[] = "A_button changed\r\n";
    char message_B[] = "B_button changed\r\n";
    uint8_t i_message_A = 0;
    uint8_t i_message_B = 0;

    while(1){
        if(pressed_1_time == 3){
            pressed_1_time = 1;
        }

        if(pressed_time == 3){
            pressed_time = 1;
        }

        // button 1
        bool current_reading_1 = (PINB & (1 << PINB4)) != 0;

        //消抖系统
        if(current_reading_1 != last_reading_1){
            stable_count_1 = 0;
            last_reading_1 = current_reading_1;
        } else if (current_reading_1 == last_reading_1){
            if(stable_count_1 < debounce_delay){
                stable_count_1 ++;
            }

            if(stable_count_1 >= debounce_delay && confirmed_state_1 != current_reading_1){
                last_confirmed_state_1 = confirmed_state_1;
                confirmed_state_1 = current_reading_1;

                if(last_confirmed_state_1 == false && confirmed_state_1 == true){
                    pressed_1 = false;
                } else if (last_confirmed_state_1 == true && confirmed_state_1 == false){
                    pressed_1 = true;

                    // ———————————————— 打印状态改变 ——————————————————
                    while(message_B[i_message_B] != '\0'){
                        usart0_send_byte(message_B[i_message_B]);
                        i_message_B ++;
                    }

                    if(message_B[i_message_B] == '\0'){
                        i_message_B = 0;
                    }

                    // —————————————————— 打印 按下次数 ————————————————————
                    press_account_B ++;
                    int8_t highdigit = check_highdigit(press_account_B);
                    index_B = 0;

                    for(;highdigit >= 0; highdigit--){
                        uint16_t divisor_B = (uint16_t)pow(10, highdigit);
                        uint8_t digit_B = (press_account_B / divisor_B) % 10;
                        press_account_BB[index_B] = digit_B + '0';
                        index_B++;
                    }

                    // uint8_t i_press_account_B = 0;
                    // while(press_account_BB[i_press_account_B] != '\0'){
                    //     usart0_send_byte(press_account_BB[i_press_account_B]);
                    //     i_press_account_B ++;
                    // }
                    for (int i = check_highdigit(press_account_B) + 1; i >= 1; i--) {
                        usart0_send_byte(press_account_BB[index_B - i]);
                    }
                    usart0_send_byte('\r');
                    usart0_send_byte('\n');

                    pressed_1_time++;   
                }
            }
        }

        // button 2
        bool current_reading = (PINB & (1 << PINB5)) != 0;

        //消抖系统
        if(current_reading != last_reading){
            stable_count = 0;
            last_reading = current_reading;
        } else if (current_reading == last_reading){
            if(stable_count < debounce_delay){
                stable_count ++;
            }

            if(stable_count >= debounce_delay && confirmed_state != current_reading){
                last_confirmed_state = confirmed_state;
                confirmed_state = current_reading;

                if(last_confirmed_state == false && confirmed_state == true){
                    pressed = false;
                } else if (last_confirmed_state == true && confirmed_state == false){
                    pressed = true;
                    
                    // ———————————————— 打印状态改变 ——————————————————
                    while(message_A[i_message_A] != '\0'){
                        usart0_send_byte(message_A[i_message_A]);
                        i_message_A ++;
                    }

                    if(message_A[i_message_A] == '\0'){
                        i_message_A = 0;
                    }

                    // —————————————————— 打印 按下次数 ————————————————————
                    press_account_A ++;
                    int8_t highdigit = check_highdigit(press_account_A);
                    index_A = 0;

                    for(;highdigit >= 0; highdigit--){
                        uint16_t divisor_A = (uint16_t)pow(10, highdigit);
                        uint8_t digit_A = (press_account_A / divisor_A) % 10;
                        press_account_AA[index_A] = digit_A + '0';
                        index_A++;
                    }

                    // uint8_t i_press_account_A = 0;
                    // while(press_account_AA[i_press_account_A] != '\0'){
                    //     usart0_send_byte(press_account_AA[i_press_account_A]);
                    //     i_press_account_A ++;
                    // }
                    for (int i = check_highdigit(press_account_A) + 1; i >= 1; i--) {
                        usart0_send_byte(press_account_AA[index_A - i]);
                    }
                    usart0_send_byte('\r');
                    usart0_send_byte('\n');

                    pressed_time ++;
                }
            }
        }

        // 按钮防抖动阻塞计时
        _delay_ms(1);

        if(pressed_1 == true && pressed_1_time == 1){
            PORTB |= (1 << PORTB6);
        } else if (pressed_1 == false && pressed_1_time == 2){
            PORTB &= ~(1 << PORTB6);
        }

        if(pressed == false && pressed_time == 2){
            PORTB &= ~(1 << PORTB7);
        } else if(pressed == true && pressed_time == 1){
            PORTB |= (1 << PORTB7);
        }
    }
}

