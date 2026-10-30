#include "usart.h"
#include <avr/io.h>

void usart0_init(void)
{
    // 9600 baud，16 MHz，普通异步模式
    UBRR0H = 0;
    UBRR0L = 103;

    // 开启接收器和发送器
    UCSR0B = (1 << RXEN0) |
             (1 << TXEN0);

    // 8个数据位、无奇偶校验、1个停止位
    UCSR0C = (1 << UCSZ01) |
             (1 << UCSZ00);
}

uint8_t usart0_rx_read(void)
{
    // 等待接收完成
    while(!(UCSR0A & (1 << RXC0))){
    }

    return UDR0;
}

void usart0_tx_send(uint8_t data)
{
    // 等待发送缓冲区为空
    while(!(UCSR0A & (1 << UDRE0))){
    }

    UDR0 = data;
}

void usart0_send_string(const char text[])
{
    uint16_t index = 0;

    while(text[index] != '\0'){
        usart0_tx_send(text[index]);
        index++;
    }
}

void usart0_send_uint32(uint32_t number)
{
    // uint32_t最多有10位十进制数字
    char digits[10];
    uint8_t digit_count = 0;

    /*
     * 先从个位开始拆数字。
     * 使用do-while可以保证number为0时也发送'0'。
     */
    do {
        uint8_t digit = number % 10;
        digits[digit_count] = digit + '0';

        digit_count++;
        number /= 10;
    } while(number > 0);

    // 数字存储顺序是反的，因此从后向前发送
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