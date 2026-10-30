#include "usart0.h"
#include <avr/io.h>

void usart0_init(void){
    UCSR0B = (1 << RXEN0)|(1 << TXEN0);
    UCSR0C = (1 << UCSZ01) | (1 << UCSZ00);

    UBRR0H = 0;
    UBRR0L = 103;
}

uint8_t usart0_rx_read(void){
    while(!(UCSR0A & (1 << RXC0))){
    }
    return UDR0;
}

void usart0_tx_send(uint8_t data){
    while(!(UCSR0A & (1 << UDRE0))){
    }
    UDR0 = data;
}

uint32_t check_highdigit(uint32_t number)
{
    uint32_t i = 0;

    while(number >= 10){
        number /= 10;
        i++;
    }

    return i;
}


