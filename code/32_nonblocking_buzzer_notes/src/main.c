#include <avr/io.h>
#include <stdbool.h>
#include <avr/interrupt.h>
#include <util/atomic.h>
#include "usart.h"
#include "timer.h"
#include "command.h"
#include "config.h"
#include "fault.h"
#include "motor.h"
#include "safety.h"
#include "homing.h"
#include "position.h"
#include "repeat.h"
#include "demo.h"
#include "report.h"
#include "nh3.h"
#include "led.h"
#include "pot.h"
#include "analog.h"
#include "light.h"
#include "buzzer.h"
#include "command/command_motor.h"
#include <avr/pgmspace.h>

volatile uint32_t system_ms = 0;
ISR(TIMER3_COMPA_vect)
{
    system_ms++;
    safety_guard_isr();
}

int main(void){
    usart0_init();
    config_init();
    motor_init();
    safety_init();
    timer3_init(1000);
    nh3_init(0);
    bool led_ready = led_init();
    bool adc_ready = analog_init(0);
    pot_init();
    light_init();
    buzzer_init();

    uint32_t now = 0;

    sei();

    usart0_tx_set_nonblocking(true);

    usart0_send_string_P(PSTR("BOOT, motor stopped, type help"));
    usart0_send_newline();

    if(!adc_ready){
        usart0_send_string_P(PSTR("WARNING ADC scan init failed"));
        usart0_send_newline();
    }

    if(!led_ready){
        usart0_send_string_P(PSTR("WARNING LED PWM init failed"));
        usart0_send_newline();
    }

    if(!config_check()){
        fault_set(FAULT_CONFIG, 0);
    }

    char command[COMMAND_LEN] = "";

    while(1){
        ATOMIC_BLOCK(ATOMIC_RESTORESTATE){
            now = system_ms;
        }

        safety_update(now);

        if(command_receive(command)){
            command_motor_execute(command, now);
        }

        motor_update(now);
        homing_update(now);
        position_update(now);
        repeat_update(now);
        demo_update(now);
        nh3_update(now);
        analog_update(now);
        pot_update();
        light_update(now);
        led_update(now);
        buzzer_update(now);

        report_update(now);
    }
}
