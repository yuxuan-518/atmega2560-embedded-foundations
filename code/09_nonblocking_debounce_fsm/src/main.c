#include <avr/io.h>
#include <stdbool.h>
#include <util/delay.h>


int main(void){
    // button input pull-up。   PB4 D10
    DDRB &= ~(1 << DDB4);
    PORTB |= (1 << PORTB4);

    // LED OUTPUT NOTLIGHT.     PB5 D11
    DDRB |= (1 << DDB5);
    PORTB &= ~(1 << PORTB5);

    // true == 1,  false == 0
    // button 1 没按下 ； 0 按下
    bool last_reading = true;
    bool confirmed_state = true;
    uint8_t stable_count = 0;
    uint8_t debounceDelay = 50;
    uint8_t edge = 0;

    while(1){
        bool current_reading = (PINB & (1 << PINB4)) != 0; 

        if(edge == 4){
            edge = 0;
        }

        if(current_reading != last_reading){
            stable_count = 0;
            last_reading = current_reading;
        } else if(current_reading == last_reading){
            if(stable_count < debounceDelay){
                stable_count ++;
                _delay_ms(1);
            }

            if(stable_count == debounceDelay){
                if(confirmed_state != current_reading){
                    confirmed_state = current_reading;
                    edge++;
                }
            }
        }

        if(edge == 1){
            PORTB |= (1 << PORTB5);
        } else if (edge == 3){
            PORTB &= ~(1 << PORTB5);
        }
    }
}