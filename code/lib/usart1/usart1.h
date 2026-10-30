#ifndef USART1_H
#define USART1_H

#include <stdint.h>
#include <stdbool.h>

#define USART1_MIN_BAUD 1200UL
#define USART1_MAX_BAUD 115200UL

typedef enum {
    USART1_RX_NONE,
    USART1_RX_BYTE,
    USART1_RX_GAP
} usart1_rx_t;

bool usart1_init(uint32_t baud);
usart1_rx_t usart1_rx_get(uint8_t *data);
void usart1_tx_send(uint8_t data);
uint16_t usart1_take_overflow(void);
uint16_t usart1_take_errors(void);

#endif
