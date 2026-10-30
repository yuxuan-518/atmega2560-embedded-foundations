#include "usart1.h"
#include <avr/io.h>
#include <avr/interrupt.h>
#include <util/atomic.h>

#define USART1_RX_BUFFER_SIZE 128
#define USART1_TX_BUFFER_SIZE 32

static volatile uint8_t rx_buffer[USART1_RX_BUFFER_SIZE];
static volatile uint8_t rx_head = 0;
static volatile uint8_t rx_tail = 0;
static volatile bool rx_gap_pending = false;
static volatile uint8_t rx_gap_pos = 0;
static volatile uint16_t rx_overflow = 0;
static volatile uint16_t rx_errors = 0;

static volatile uint8_t tx_buffer[USART1_TX_BUFFER_SIZE];
static volatile uint8_t tx_head = 0;
static volatile uint8_t tx_tail = 0;

static void usart1_mark_gap(void)
{
    if(!rx_gap_pending){
        rx_gap_pending = true;
        rx_gap_pos = rx_head;
    }
}

ISR(USART1_RX_vect)
{
    uint8_t status = UCSR1A;
    uint8_t data = UDR1;

    if(status & ((1 << FE1) | (1 << DOR1))){
        if(rx_errors < 65535){
            rx_errors++;
        }
        usart1_mark_gap();

        if(status & (1 << FE1)){
            return;
        }
    }

    uint8_t next = (rx_head + 1) & (USART1_RX_BUFFER_SIZE - 1);

    if(next == rx_tail){
        if(rx_overflow < 65535){
            rx_overflow++;
        }
        usart1_mark_gap();
        return;
    }

    rx_buffer[rx_head] = data;
    rx_head = next;
}

ISR(USART1_UDRE_vect)
{
    if(tx_head == tx_tail){
        UCSR1B &= ~(1 << UDRIE1);
        return;
    }

    UDR1 = tx_buffer[tx_tail];
    tx_tail = (tx_tail + 1) & (USART1_TX_BUFFER_SIZE - 1);
}

bool usart1_init(uint32_t baud)
{
    if(baud < USART1_MIN_BAUD || baud > USART1_MAX_BAUD){
        return false;
    }

    uint32_t ubrr = (F_CPU + 4UL * baud) / (8UL * baud) - 1;

    ATOMIC_BLOCK(ATOMIC_RESTORESTATE){
        UCSR1B = 0;

        UBRR1H = (ubrr >> 8) & 0x0F;
        UBRR1L = ubrr & 0xFF;

        UCSR1A = (1 << U2X1);
        UCSR1C = (1 << UCSZ11) |
                 (1 << UCSZ10);

        rx_head = 0;
        rx_tail = 0;
        rx_gap_pending = false;
        tx_head = 0;
        tx_tail = 0;

        UCSR1B = (1 << RXEN1) |
                 (1 << TXEN1) |
                 (1 << RXCIE1);
    }

    return true;
}

usart1_rx_t usart1_rx_get(uint8_t *data)
{
    usart1_rx_t result = USART1_RX_NONE;

    ATOMIC_BLOCK(ATOMIC_RESTORESTATE){
        if(rx_gap_pending && rx_tail == rx_gap_pos){
            rx_gap_pending = false;
            result = USART1_RX_GAP;
        } else if(rx_head != rx_tail){
            *data = rx_buffer[rx_tail];
            rx_tail = (rx_tail + 1) & (USART1_RX_BUFFER_SIZE - 1);
            result = USART1_RX_BYTE;
        }
    }

    return result;
}

void usart1_tx_send(uint8_t data)
{
    ATOMIC_BLOCK(ATOMIC_RESTORESTATE){
        uint8_t next = (tx_head + 1) & (USART1_TX_BUFFER_SIZE - 1);

        if(next != tx_tail){
            tx_buffer[tx_head] = data;
            tx_head = next;
            UCSR1B |= (1 << UDRIE1);
        }
    }
}

uint16_t usart1_take_overflow(void)
{
    uint16_t count = 0;

    ATOMIC_BLOCK(ATOMIC_RESTORESTATE){
        count = rx_overflow;
        rx_overflow = 0;
    }

    return count;
}

uint16_t usart1_take_errors(void)
{
    uint16_t count = 0;

    ATOMIC_BLOCK(ATOMIC_RESTORESTATE){
        count = rx_errors;
        rx_errors = 0;
    }

    return count;
}
