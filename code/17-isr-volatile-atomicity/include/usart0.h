#ifndef USART0_H
#define USART0_H

#include <stdint.h>

void usart0_init(void);
uint8_t usart0_rx_read(void);
void usart0_tx_send(uint8_t data);
uint32_t check_highdigit(uint32_t number);

#endif