#include <avr/io.h>
#include <stdbool.h>
#include <avr/interrupt.h>
#include <util/atomic.h>
#include "usart.h"
#include "timer.h"
#include "command.h"
#include "motor.h"
#include "demo.h"
#include "command/command_motor.h"

volatile uint32_t system_ms = 0;
ISR(TIMER3_COMPA_vect)
{
    system_ms++;
}

int main(void){
    usart0_init();
    motor_init();
    timer3_init(1000);

    uint32_t now = 0;

    sei();

    char command[COMMAND_LEN] = "";

    uint32_t last_serial = 0;
    uint32_t last_log = 0;
    motor_state_t last_state = MOTOR_STOPPED;

    usart0_send_string("BOOT, type help");
    usart0_send_newline();

    while(1){
        ATOMIC_BLOCK(ATOMIC_RESTORESTATE){
            now = system_ms;
        }

        if(command_receive(command)){
            command_motor_execute(command);
        }

        motor_update(now);
        demo_update(now);

        motor_state_t state = motor_get_state();
        if(state != last_state){
            last_state = state;
            usart0_send_string("[");
            usart0_send_uint32(now);
            usart0_send_string(" ms] ");
            command_motor_send_state(state);
            usart0_send_string(" ");
            usart0_send_uint32(motor_get_current_hz());
            usart0_send_string(" Hz");
            usart0_send_newline();
        }

        if(command_motor_log_enabled() && now - last_log >= 100){
            last_log = now;
            usart0_send_string("LOG,");
            usart0_send_uint32(now);
            usart0_send_string(",");
            usart0_send_uint32(motor_get_current_hz());
            usart0_send_newline();
        }

        if(now - last_serial >= 1000){
            last_serial = now;
            usart0_send_string("TIME: ");
            usart0_send_uint32(now / 1000);
            usart0_send_string(" s, ");
            command_motor_send_status();
        }
    }
}
