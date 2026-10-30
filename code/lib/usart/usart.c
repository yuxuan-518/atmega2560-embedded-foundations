#include "usart.h"
#include <avr/io.h>
#include <avr/interrupt.h>
#include <util/atomic.h>
#include <avr/pgmspace.h>

#define USART0_RX_BUFFER_SIZE 32

#ifndef USART0_TX_BUFFER_SIZE
#define USART0_TX_BUFFER_SIZE 512
#endif

#if (USART0_TX_BUFFER_SIZE & (USART0_TX_BUFFER_SIZE - 1)) != 0
#error "USART0_TX_BUFFER_SIZE must be a power of 2"
#endif

static volatile uint8_t rx_buffer[USART0_RX_BUFFER_SIZE];
static volatile uint8_t rx_head = 0;
static volatile uint8_t rx_tail = 0;

static volatile uint8_t tx_buffer[USART0_TX_BUFFER_SIZE];
static volatile uint16_t tx_head = 0;
static volatile uint16_t tx_tail = 0;
static volatile bool tx_nonblocking = false;
static volatile uint16_t tx_dropped = 0;

ISR(USART0_RX_vect)
{
    uint8_t data = UDR0;
    uint8_t next = (rx_head + 1) & (USART0_RX_BUFFER_SIZE - 1);

    if(next != rx_tail){
        rx_buffer[rx_head] = data;
        rx_head = next;
    }
}

ISR(USART0_UDRE_vect)
{
    if(tx_head == tx_tail){
        UCSR0B &= ~(1 << UDRIE0);
        return;
    }

    UDR0 = tx_buffer[tx_tail];
    tx_tail = (tx_tail + 1) & (USART0_TX_BUFFER_SIZE - 1);
}

static uint16_t usart0_tx_read_tail(void)
{
    uint16_t tail = 0;

    ATOMIC_BLOCK(ATOMIC_RESTORESTATE){
        tail = tx_tail;
    }

    return tail;
}

void usart0_init(void)
{
    UBRR0H = 0;
    UBRR0L = 103;

    UCSR0B = (1 << RXEN0) |
             (1 << TXEN0) |
             (1 << RXCIE0);

    UCSR0C = (1 << UCSZ01) |
             (1 << UCSZ00);
}

bool usart0_rx_ready(void)
{
    return rx_head != rx_tail;
}

uint8_t usart0_rx_read(void)
{
    while(rx_head == rx_tail){
    }

    uint8_t data = rx_buffer[rx_tail];
    rx_tail = (rx_tail + 1) & (USART0_RX_BUFFER_SIZE - 1);

    return data;
}

void usart0_tx_send(uint8_t data)
{
    if(!(SREG & (1 << SREG_I))){
        while(tx_head != tx_tail){
            while(!(UCSR0A & (1 << UDRE0))){
            }
            UDR0 = tx_buffer[tx_tail];
            tx_tail = (tx_tail + 1) & (USART0_TX_BUFFER_SIZE - 1);
        }

        while(!(UCSR0A & (1 << UDRE0))){
        }
        UDR0 = data;
        return;
    }

    uint16_t next = (tx_head + 1) & (USART0_TX_BUFFER_SIZE - 1);

    if(next == usart0_tx_read_tail()){
        if(tx_nonblocking){
            if(tx_dropped < 65535){
                tx_dropped++;
            }
            return;
        }

        while(next == usart0_tx_read_tail()){
        }
    }

    tx_buffer[tx_head] = data;

    ATOMIC_BLOCK(ATOMIC_RESTORESTATE){
        tx_head = next;
        UCSR0B |= (1 << UDRIE0);
    }
}

void usart0_send_string(const char text[])
{
    uint16_t index = 0;

    while(text[index] != '\0'){
        usart0_tx_send(text[index]);
        index++;
    }
}

void usart0_send_string_P(const char *text)
{
    char c = pgm_read_byte(text);

    while(c != '\0'){
        usart0_tx_send(c);
        text++;
        c = pgm_read_byte(text);
    }
}

void usart0_send_uint32(uint32_t number)
{
    char digits[10];
    uint8_t digit_count = 0;

    do {
        uint8_t digit = number % 10;
        digits[digit_count] = digit + '0';

        digit_count++;
        number /= 10;
    } while(number > 0);

    while(digit_count > 0){
        digit_count--;
        usart0_tx_send(digits[digit_count]);
    }
}

void usart0_send_newline(void)
{
    usart0_tx_send('\r');
    usart0_tx_send('\n');
}

void usart0_tx_set_nonblocking(bool enable)
{
    tx_nonblocking = enable;
}

uint16_t usart0_tx_free(void)
{
    uint16_t head = 0;
    uint16_t tail = 0;

    ATOMIC_BLOCK(ATOMIC_RESTORESTATE){
        head = tx_head;
        tail = tx_tail;
    }

    return (tail - head - 1) & (USART0_TX_BUFFER_SIZE - 1);
}

uint16_t usart0_tx_take_dropped(void)
{
    uint16_t dropped = 0;

    ATOMIC_BLOCK(ATOMIC_RESTORESTATE){
        dropped = tx_dropped;
        tx_dropped = 0;
    }

    return dropped;
}
