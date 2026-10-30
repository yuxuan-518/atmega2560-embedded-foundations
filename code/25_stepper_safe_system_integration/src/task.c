#include "task.h"
#include "motor.h"
#include "homing.h"
#include "demo.h"
#include "position.h"
#include "repeat.h"
#include "usart.h"

bool task_busy(void)
{
    return motor_is_moving() || homing_busy() || demo_is_running() || position_busy() || repeat_busy();
}

void task_abort_all(void)
{
    repeat_abort();
    demo_abort();
    homing_abort();
    position_abort();
    motor_stop_now();
}

void task_send_name(void)
{
    if(repeat_busy()){
        usart0_send_string("REPEAT");
    } else if(position_busy()){
        usart0_send_string("MOVE");
    } else if(homing_busy()){
        usart0_send_string("HOME_");
        homing_send_phase();
    } else if(demo_is_running()){
        usart0_send_string("DEMO");
    } else if(motor_is_moving()){
        usart0_send_string("RUN");
    } else {
        usart0_send_string("IDLE");
    }
}
