#ifndef USART_H
#define USART_H

#include <stdint.h>
#include <stdbool.h>

void usart0_init(void);
bool usart0_rx_ready(void);
uint8_t usart0_rx_read(void);
void usart0_tx_send(uint8_t data);
void usart0_send_string(const char text[]);
void usart0_send_string_P(const char *text);
void usart0_send_uint32(uint32_t number);
void usart0_send_newline(void);

void usart0_tx_set_nonblocking(bool enable);
uint16_t usart0_tx_free(void);
uint16_t usart0_tx_take_dropped(void);

#endif
