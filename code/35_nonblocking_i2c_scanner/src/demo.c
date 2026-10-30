#include "demo.h"
#include "motor.h"
#include "usart.h"
#include <avr/pgmspace.h>

static uint8_t demo_phase = 0;
static uint32_t cruise_start = 0;

void demo_start(void)
{
    if(motor_get_state() != MOTOR_STOPPED){
        usart0_send_string_P(PSTR("WARNING stop motor before demo"));
        usart0_send_newline();
        return;
    }

    motor_start();
    demo_phase = 1;

    usart0_send_string_P(PSTR("DEMO START"));
    usart0_send_newline();
}

void demo_update(uint32_t now)
{
    motor_state_t state = motor_get_state();

    if(demo_phase == 1){
        if(state == MOTOR_CRUISE){
            cruise_start = now;
            demo_phase = 2;
        } else if(state == MOTOR_STOPPED || state == MOTOR_STOPPING || state == MOTOR_REVERSING){
            demo_phase = 0;
            usart0_send_string_P(PSTR("DEMO ABORT"));
            usart0_send_newline();
        }
    } else if(demo_phase == 2){
        if(state != MOTOR_CRUISE){
            demo_phase = 0;
            usart0_send_string_P(PSTR("DEMO ABORT"));
            usart0_send_newline();
        } else if(now - cruise_start >= DEMO_CRUISE_MS){
            motor_stop();
            demo_phase = 3;
        }
    } else if(demo_phase == 3){
        if(state == MOTOR_STOPPED){
            demo_phase = 0;
            usart0_send_string_P(PSTR("DEMO DONE"));
            usart0_send_newline();
        } else if(state != MOTOR_STOPPING){
            demo_phase = 0;
            usart0_send_string_P(PSTR("DEMO ABORT"));
            usart0_send_newline();
        }
    }
}

bool demo_is_running(void)
{
    return demo_phase != 0;
}

void demo_abort(void)
{
    if(demo_phase == 0){
        return;
    }

    demo_phase = 0;
    usart0_send_string_P(PSTR("DEMO ABORT"));
    usart0_send_newline();
}
