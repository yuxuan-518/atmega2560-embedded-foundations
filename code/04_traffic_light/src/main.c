#include <avr/io.h>
#include <util/delay.h>
#include <stdint.h>

void wait_ms(uint16_t duration_ms){
    for(uint16_t i = 0; i < duration_ms; i++){
        _delay_ms(1);
    }
}

int main(void){
    DDRB |= ((1 << DDB4)|(1 << DDB5)|(1 << DDB6));

    const uint8_t red_states[6] = {
    1,
    0,
    0,
    0,
    0,
    0
    };

    const uint8_t green_states[6] = {
        0,
        0,
        1,
        0,
        0,
        0
    };

    const uint8_t blue_states[6] = {
        0,
        0,
        0,
        0,
        1,
        0
    };

    const uint16_t durations_ms[6] = {
    50, 20, 50, 20, 50, 20
};

    while(1){
        for(uint8_t i = 0; i < 6; i++){
            if(red_states[i]){
                PORTB |= (1 << PORTB4);
            } else {
                PORTB &= ~(1 << PORTB4);
            }

            if(green_states[i]){
                PORTB |= (1 << PORTB5);
            } else {
                PORTB &= ~(1 << PORTB5);
            }

            if(blue_states[i]){
                PORTB |= (1 << PORTB6);
            } else {
                PORTB &= ~(1 << PORTB6);
            }

            wait_ms(durations_ms[i]);
        }
    }
}
    