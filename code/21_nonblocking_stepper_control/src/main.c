#include <avr/io.h>
#include <stdbool.h>
#include <avr/interrupt.h>
#include <util/atomic.h>
#include "usart.h"
#include "timer.h"
#include "motor.h"
#include "command/command.h"
#include "command/command_motor.h"

volatile uint32_t system_ms = 0;
ISR(TIMER3_COMPA_vect)
{
    system_ms++;
}

int main(void){
    // ............usart............
    usart0_init();

    motor_init();

    // timer init
    timer3_init(1000);

    // 隔离 防止复制 s 数的时候被吞掉另一节
    uint32_t now = 0;

    sei();      // 打开全局中断总开关

    char command[COMMAND_LEN] = "";
    uint32_t last_serial = 0;

    usart0_send_string("BOOT, type help");
    usart0_send_newline();

    while(1){
        ATOMIC_BLOCK(ATOMIC_RESTORESTATE){
            now = system_ms;
        }

        if(command_receive(command)){
            command_motor_execute(command);
        }

        if(now - last_serial >= 2000){
            last_serial = now;
            usart0_send_string("TIME: ");
            usart0_send_uint32(now / 1000);
            usart0_send_string(" s, ");
            command_motor_send_status();
        }
    }
}
