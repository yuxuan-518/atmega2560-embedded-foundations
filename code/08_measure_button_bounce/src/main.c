#include <avr/io.h>
#include <stdbool.h>
#include <util/delay.h>

void blink(uint8_t times){
    for(uint8_t i = 0; i < times; i++){
        PORTB |= (1 << PORTB5);
        _delay_ms(200);
        PORTB &= ~(1 << PORTB5);
        _delay_ms(200);
    }
}

int main(void){
    // button input pull-up。   PB4 D10
    DDRB &= ~(1 << DDB4);
    PORTB |= (1 << PORTB4);

    // LED OUTPUT NOTLIGHT.     PB5 D11
    DDRB |= (1 << DDB5);
    PORTB &= ~(1 << PORTB5);

    bool last_button = true;

    while(1){
        bool current_button = PINB & (1 << PINB4);
        // bool current_led = PINB & (1 << PINB5);
        uint8_t total_transition;

        if(current_button != last_button){
            if(current_button == false){
                total_transition = 1;
                last_button = false;

                for(uint8_t sample_index = 0; sample_index < 100; sample_index++){
                    _delay_us(50);

                    current_button = (PINB & (1 << PINB4));

                    if(current_button != last_button){
                        total_transition++;
                    }

                    last_button = current_button;
                }

                while ((PINB & (1 << PINB4)) == 0) {
                }

                blink(total_transition);

                last_button = true;
            }
        }
    }
}