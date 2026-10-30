#include "task.h"
#include "motor.h"
#include "homing.h"
#include "demo.h"
#include "position.h"
#include "repeat.h"
#include "autorun.h"
#include "usart.h"
#include <avr/pgmspace.h>

bool task_busy(void)
{
    return autorun_busy() || motor_is_moving() || homing_busy() || demo_is_running() || position_busy() || repeat_busy();
}

void task_abort_all(void)
{
    autorun_abort();
    repeat_abort();
    demo_abort();
    homing_abort();
    position_abort();
    motor_stop_now();
}

void task_stop(void)
{
    autorun_stop();
    repeat_abort();
    demo_abort();

    if(homing_busy()){
        homing_abort();
    } else {
        motor_stop();
    }
}

void task_send_name(void)
{
    if(autorun_busy()){
        usart0_send_string_P(PSTR("AUTO"));
    } else if(repeat_busy()){
        usart0_send_string_P(PSTR("REPEAT"));
    } else if(position_busy()){
        usart0_send_string_P(PSTR("MOVE"));
    } else if(homing_busy()){
        usart0_send_string_P(PSTR("HOME_"));
        homing_send_phase();
    } else if(demo_is_running()){
        usart0_send_string_P(PSTR("DEMO"));
    } else if(motor_is_moving()){
        usart0_send_string_P(PSTR("RUN"));
    } else {
        usart0_send_string_P(PSTR("IDLE"));
    }
}
