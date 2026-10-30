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
#include "counter.h"
#include "ustimer.h"
#include "ultrasonic.h"
#include "range.h"
#include "i2c.h"
#include "scanner.h"
#include "imu.h"
#include "spi.h"
#include "loopback.h"
#include "dac.h"
#include "display.h"
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
    counter_init();
    ustimer_init();
    ultrasonic_init();
    range_init();
    bool i2c_ready = i2c_init(CONFIG_I2C_HZ, CONFIG_I2C_PULLUP, CONFIG_I2C_TIMEOUT_MS);
    scanner_init();
    imu_init();
    bool spi_ready = spi_init(CONFIG_SPI_MISO_PULLUP, CONFIG_SPI_TIMEOUT_MS);
    loopback_init();
    bool dac_ready = dac_init();
    bool display_ready = display_init(0);

    uint32_t now = 0;

    sei();

    usart0_tx_set_nonblocking(true);

    usart0_send_string_P(PSTR("BOOT, motor stopped, type help"));
    usart0_send_newline();

    if(!adc_ready){
        usart0_send_string_P(PSTR("WARNING ADC scan init failed"));
        usart0_send_newline();
    }

    if(!i2c_ready){
        usart0_send_string_P(PSTR("WARNING I2C init failed"));
        usart0_send_newline();
    }

    if(!spi_ready){
        usart0_send_string_P(PSTR("WARNING SPI init failed"));
        usart0_send_newline();
    }

    if(!dac_ready){
        usart0_send_string_P(PSTR("WARNING DAC SPI config invalid, MCP4921 needs mode 0 or 3"));
        usart0_send_newline();
    }

    if(!display_ready){
        usart0_send_string_P(PSTR("WARNING display address must be 0x3C or 0x3D"));
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
        counter_update(now);
        range_update(now);
        scanner_update(now);
        imu_update(now);
        loopback_update(now);
        dac_update(now);
        display_update(now);

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
