#include <avr/io.h>
#include <stdbool.h>
#include <util/delay.h>
#include <math.h>

// _________________________________ usart _________________________________
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



// ___________________________________________________________________________________________________
bool too_long_status = false;

int main(void){
    DDRB = DDRB | (1 << DDB6);
    PORTB &= ~(1 << PORTB6);

    char array[16] = "";
    uint16_t index_read = 0;

    usart0_init();
    
    while(1){
        uint8_t read = usart0_rx_read();
        
        if (read == '\x08') {
            if(index_read > 0){
                index_read--;
                array[index_read] = '\0';
            }
        } else if (read != '\r' && read != '\n') {
            if(index_read < 15){
                array[index_read] = read;
                index_read++;
                array[index_read] = '\0';
            } else {
                too_long_status = true;
            }
        }

        uint16_t index_send = 0;
        if(read == '\n'){
            if(too_long_status == true){
                char too_long[] = "command unlegal";
                uint8_t index_too_long = 0;

                while(too_long[index_too_long] != '\0'){
                    usart0_tx_send(too_long[index_too_long]);
                    index_too_long++;
                }
                usart0_tx_send('\r');
                usart0_tx_send('\n');
            } else {
                    while(array[index_send] != '\0'){
                        usart0_tx_send(array[index_send]);
                        index_send++;
                    }
                    usart0_tx_send('\r');
                    usart0_tx_send('\n');

                    // —————————————————————————————————————————————— on ——————————————————————————————————————————————————
                    bool ON_state = true;
                    char buzzer_on_1[16] = "on";
                    for(int i = 0; i <16; i++){
                        if(array[i] != buzzer_on_1[i]){
                            ON_state = false;
                            break;
                        }
                    }
                    if(ON_state == true){
                        PORTB |= (1 << PORTB6);
                    }

                    // —————————————————————————————————————————————— off_1 ——————————————————————————————————————————————————
                    bool OFF_state = true;
                    char buzzer_off_1[16] = "off";
                    for(int i = 0; i <16; i++){
                        if(array[i] != buzzer_off_1[i]){
                            OFF_state = false;
                            break;
                        }
                    }
                    if(OFF_state == true){
                        PORTB &= ~(1 << PORTB6);
                    }

                // —————————————————————————————————————— 归零 ————————————————————————————————————————
                for (int i = 0; i < 16; i++) {
                    array[i] = '\0';
                }
                index_read = 0;
                too_long_status = false;
            }
        }
    
    }
}