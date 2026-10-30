#include <avr/io.h>
#include <stdbool.h>
#include <util/delay.h>

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