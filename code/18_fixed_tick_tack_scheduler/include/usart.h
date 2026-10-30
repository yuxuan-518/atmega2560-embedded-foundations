#ifndef USART_H
#define USART_H

#include <stdint.h>

// 初始化USART0：9600 baud、8N1、开启收发
void usart0_init(void);

// 接收一个byte
uint8_t usart0_rx_read(void);

// 发送一个byte
void usart0_tx_send(uint8_t data);

// 发送以'\0'结尾的字符串
void usart0_send_string(const char text[]);

// 发送uint32_t十进制数字
void usart0_send_uint32(uint32_t number);

// 发送\r\n
void usart0_send_newline(void);

#endif