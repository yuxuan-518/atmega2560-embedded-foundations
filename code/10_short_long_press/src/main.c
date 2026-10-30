#include <avr/io.h>
#include <stdbool.h>
#include <util/delay.h>

void blink_slow(uint8_t time){
    for(int i = 0; i < time; i++){
        PORTB |= (1 << PORTB6);
        _delay_ms(150);
        PORTB &= ~(1 << PORTB6);
        _delay_ms(150);
    }
}

void blink_fast(uint8_t time){
    for(int i = 0; i < time; i++){
        PORTB |= (1 << PORTB6);
        _delay_ms(50);
        PORTB &= ~(1 << PORTB6);
        _delay_ms(50);
    }
}

int main(void){
    // button input pull-up。   PB4 D10
    DDRB &= ~(1 << DDB4);
    PORTB |= (1 << PORTB4);

    // LED OUTPUT NOTLIGHT.     PB5 D11
    DDRB |= (1 << DDB6);
    PORTB &= ~(1 << PORTB6);

    // true == 1,  false == 0
    // button 1 没按下 ； 0 按下
    
    //消抖系统变量：
    bool last_reading = true;
    bool confirmed_state = true;
    bool last_confirmed_state = false;
    bool pressed = false;
    uint8_t stable_count = 0;
    uint8_t debounce_delay = 50;
    //计时系统变量：
    uint16_t time = 0;

    while(1){
        bool current_reading = (PINB & (1 << PINB4)) != 0;

        //消抖系统
        if(current_reading != last_reading){
            stable_count = 0;
            last_reading = current_reading;
        } else if (current_reading == last_reading){
            if(stable_count < debounce_delay){
                stable_count ++;
            }
            _delay_ms(1);

            if(stable_count >= debounce_delay && confirmed_state != current_reading){
                last_confirmed_state = confirmed_state;
                confirmed_state = current_reading;

                if(last_confirmed_state == false && confirmed_state == true){
                    pressed = false;
                } else if (last_confirmed_state == true && confirmed_state == false){
                    pressed = true;
                }
            }
        }

        //计时系统：
        if (pressed == true){
            if(time <= 250){
                time ++;
            }
        } else if (pressed == false && time != 0){
            if(time <= 250){
                blink_fast(3);
            } else if (time > 250){
                blink_slow(5);
            }
            time = 0;
        }
    }
}