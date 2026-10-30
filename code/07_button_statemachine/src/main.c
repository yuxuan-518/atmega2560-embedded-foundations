#include <avr/io.h>
#include <stdbool.h>
#include <util/delay.h>

void wait(uint16_t detime){
    for(int i = 0; i < detime; i++){
        _delay_ms(1);
    }
}

int main(void){
    DDRB |= (1 << DDB5);
    DDRB &= ~(1 << DDB4);

    PORTB |= (1 << PORTB4);

    bool last_button_state = true;

    //PORTB &= ~(1 << PORTB5);

    uint8_t mode = 0;

    while(1){
        bool current_button_state = (PINB & (1 << PINB4)) != 0;

        if(current_button_state != last_button_state){
            if(current_button_state == 0){
                mode++;

                if(mode == 4){
                    mode = 0;
                }
                
                switch(mode){
                    case 0:
                        PORTB &= ~(1 << PORTB5);
                        break;
                    case 1:
                        PORTB |= (1 << PORTB5);
                        break;
                    case 2:
                        PORTB &= ~(1 << PORTB5);
                        wait(50);
                        PORTB |= (1 << PORTB5);
                        wait(50);
                        PORTB &= ~(1 << PORTB5);
                        wait(50);
                        PORTB |= (1 << PORTB5);
                        wait(50);
                        PORTB &= ~(1 << PORTB5);
                        wait(50);
                        break;
                    case 3:
                        PORTB &= ~(1 << PORTB5);
                        wait(200);
                        PORTB |= (1 << PORTB5);
                        wait(200);
                        PORTB &= ~(1 << PORTB5);
                        wait(200);
                        PORTB |= (1 << PORTB5);
                        wait(200);
                        PORTB &= ~(1 << PORTB5);
                        wait(200);
                        break;
                }
            }
            last_button_state = current_button_state;
        }
    }
}