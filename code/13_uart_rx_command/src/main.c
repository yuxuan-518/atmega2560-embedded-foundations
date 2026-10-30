#include <avr/io.h>
#include <stdbool.h>
#include <util/delay.h>
#include <math.h>

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

int main(void){
    DDRB |= (1 << DDB7) | (1 << DDB6);

    usart0_init();

    char array[16] = "";
    uint16_t index_read = 0;

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

                    // —————————————————————————————————————————————— on_1 ——————————————————————————————————————————————————
                    bool ON1_state = true;
                    char LED_on_1[16] = "on_1";
                    for(int i = 0; i <16; i++){
                        if(array[i] != LED_on_1[i]){
                            ON1_state = false;
                            break;
                        }
                    }
                    if(ON1_state == true){
                        PORTB |= (1 << PORTB7);
                    }

                    // —————————————————————————————————————————————— off_1 ——————————————————————————————————————————————————
                    bool OFF1_state = true;
                    char LED_off_1[16] = "off_1";
                    for(int i = 0; i <16; i++){
                        if(array[i] != LED_off_1[i]){
                            OFF1_state = false;
                            break;
                        }
                    }
                    if(OFF1_state == true){
                        PORTB &= ~(1 << PORTB7);
                    }
                    
                    // —————————————————————————————————————————————— on_2 ——————————————————————————————————————————————————
                    bool ON2_state = true;
                    char LED_on_2[16] = "on_2";
                    for(int i = 0; i <16; i++){
                        if(array[i] != LED_on_2[i]){
                            ON2_state = false;
                            break;
                        }
                    }
                    if(ON2_state == true){
                        PORTB |= (1 << PORTB6);
                    }

                    // —————————————————————————————————————————————— off_2 ——————————————————————————————————————————————————
                    bool OFF2_state = true;
                    char LED_off_2[16] = "off_2";
                    for(int i = 0; i <16; i++){
                        if(array[i] != LED_off_2[i]){
                            OFF2_state = false;
                            break;
                        }
                    }
                    if(OFF2_state == true){
                        PORTB &= ~(1 << PORTB6);
                    }

                    // ———————————————————————————————————————————————— help ————————————————————————————————————————————————
                    bool help_state = true;
                    char help[16] = "help";
                    char help_content[] = "      commond      control\r\n"
                                          "        on_1       turn on led_1\r\n"
                                          "       off_1      turn off led_1\r\n"
                                          "        on_2       turn on led_2\r\n"
                                          "       off_2      turn off led_2\r\n"
                                          "      status   return led 1&2 status\r\n";
                    uint8_t index_help = 0;
                    for(int i = 0; i <16; i++){
                        if(array[i] != help[i]){
                            help_state = false;
                            break;
                        }
                    }
                    if(help_state == true){
                        while(help_content[index_help] != '\0'){
                            usart0_tx_send(help_content[index_help]);
                            index_help++;
                        }
                        usart0_tx_send('\r');
                        usart0_tx_send('\n');
                    }

                    // ———————————————————————————————————————————————— status ————————————————————————————————————————————————
                    bool status_state = true;
                    char status[16] = "status";

                    char status_on1[] = "LED1 ON NOW";
                    uint8_t index_status_on1 = 0;
                    char status_off1[] = "LED1 OFF NOW";
                    uint8_t index_status_off1 = 0;

                    char status_on2[] = "LED2 ON NOW";
                    uint8_t index_status_on2 = 0;
                    char status_off2[] = "LED2 OFF NOW";
                    uint8_t index_status_off2 = 0;

                    for(int i = 0; i <16; i++){
                        if(array[i] != status[i]){
                            status_state = false;
                            break;
                        }
                    }

                    //.  1
                    if(status_state == true){
                        bool pin_status = PINB & (1 << PINB7);

                        if(pin_status){
                            while(status_on1[index_status_on1] != '\0'){
                                usart0_tx_send(status_on1[index_status_on1]);
                                index_status_on1++;
                            }
                            usart0_tx_send('\r');
                            usart0_tx_send('\n');
                        } else {
                            while(status_off1[index_status_off1] != '\0'){
                                usart0_tx_send(status_off1[index_status_off1]);
                                index_status_off1++;
                            }
                            usart0_tx_send('\r');
                            usart0_tx_send('\n');
                        }
                    }

                    //.  2
                    if(status_state == true){
                        bool pin_status = PINB & (1 << PINB6);

                        if(pin_status){
                            while(status_on2[index_status_on2] != '\0'){
                                usart0_tx_send(status_on2[index_status_on2]);
                                index_status_on2++;
                            }
                            usart0_tx_send('\r');
                            usart0_tx_send('\n');
                        } else {
                            while(status_off2[index_status_off2] != '\0'){
                                usart0_tx_send(status_off2[index_status_off2]);
                                index_status_off2++;
                            }
                            usart0_tx_send('\r');
                            usart0_tx_send('\n');
                        }
                    }

                    // ———————————————————————————————————————————————— warning ————————————————————————————————————————————————
                    char warning[] = "!!!!!!!!!!!warning!!!!!!!!!!!!!";
                    uint8_t index_warning = 0;
                    if(OFF1_state == false && ON1_state == false && help_state == false && status_state == false && OFF2_state == false && ON2_state == false){
                        while(warning[index_warning] != '\0'){
                            usart0_tx_send(warning[index_warning]);
                            index_warning++;
                        }
                        usart0_tx_send('\r');
                        usart0_tx_send('\n');
                    }
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