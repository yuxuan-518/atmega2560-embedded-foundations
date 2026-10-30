#include "safety.h"
#include "config.h"
#include "limit.h"
#include "estop.h"
#include "fault.h"
#include "motor.h"
#include "homing.h"
#include "task.h"
#include "timer.h"
#include "usart.h"

#include <avr/interrupt.h>
#include <util/atomic.h>
#include <avr/pgmspace.h>

#define GUARD_MIN 0x01
#define GUARD_MAX 0x02

static volatile uint8_t guard_hit = 0;

ISR(PCINT2_vect)
{
    safety_guard_isr();
}

void safety_guard_isr(void)
{
    if(estop_input_active()){
        estop_latch();
        return;
    }

    if(!timer1_is_running()){
        return;
    }

    uint8_t dir = motor_get_direction();

    if(dir == MOTOR_DIR_MIN && limit_min_raw()){
        timer1_stop();
        guard_hit |= GUARD_MIN;
    } else if(dir == MOTOR_DIR_MAX && limit_max_raw()){
        timer1_stop();
        guard_hit |= GUARD_MAX;
    }
}

static void safety_send_event(uint32_t now, const char *text)
{
    usart0_send_string_P(PSTR("["));
    usart0_send_uint32(now);
    usart0_send_string_P(PSTR(" ms] "));
    usart0_send_string_P(text);
    usart0_send_newline();
}

static void safety_limit_hit(uint32_t now, const char *text)
{
    safety_send_event(now, text);

    if(homing_busy()){
        motor_stop_now();
        return;
    }

    bool was_homed = homing_is_homed();

    task_abort_all();
    homing_invalidate();

    if(was_homed){
        fault_set(FAULT_LIMIT_UNEXPECTED, now);
    }
}

void safety_init(void)
{
    limit_init();
    estop_init();
}

void safety_update(uint32_t now)
{
    uint8_t hit = 0;

    ATOMIC_BLOCK(ATOMIC_RESTORESTATE){
        hit = guard_hit;
        guard_hit = 0;
    }

    if(homing_busy() && motor_is_moving()){
        if(motor_get_direction() == MOTOR_DIR_MAX){
            hit &= (uint8_t)~GUARD_MIN;
        } else {
            hit &= (uint8_t)~GUARD_MAX;
        }
    }

    limit_update(now);

    if(estop_is_latched() || estop_input_active()){
        estop_latch();

        if(task_busy()){
            task_abort_all();
        }
        homing_invalidate();

        fault_set(FAULT_ESTOP, now);
        return;
    }

    if(limit_min_active() && limit_max_active()){
        if(task_busy()){
            task_abort_all();
        }
        homing_invalidate();

        fault_set(FAULT_LIMIT_WIRING, now);
        return;
    }

    if(fault_active()){
        if(task_busy()){
            task_abort_all();
        }
        return;
    }

    bool moving = motor_is_moving();
    uint8_t dir = motor_get_direction();

    if((hit & GUARD_MIN) || (moving && dir == MOTOR_DIR_MIN && limit_min_active())){
        safety_limit_hit(now, PSTR("LIMIT MIN HIT"));
        return;
    }

    if((hit & GUARD_MAX) || (moving && dir == MOTOR_DIR_MAX && limit_max_active())){
        safety_limit_hit(now, PSTR("LIMIT MAX HIT"));
        return;
    }

    if(moving && homing_is_homed() && !homing_busy()){
        const config_t *cfg = config_get();
        int32_t pos = motor_get_position();

        if((dir == MOTOR_DIR_MIN && pos < cfg->soft_min) ||
           (dir == MOTOR_DIR_MAX && pos > cfg->soft_max)){
            task_abort_all();
            safety_send_event(now, PSTR("SOFT LIMIT HIT"));
            fault_set(FAULT_SOFT_LIMIT, now);
        }
    }
}

bool safety_reset(void)
{
    if(limit_min_active() && limit_max_active()){
        usart0_send_string_P(PSTR("WARNING both limits active, check wiring"));
        usart0_send_newline();
        return false;
    }

    if(!config_check()){
        usart0_send_string_P(PSTR("WARNING config invalid, fix with set"));
        usart0_send_newline();
        return false;
    }

    bool cleared = false;

    ATOMIC_BLOCK(ATOMIC_RESTORESTATE){
        if(!estop_input_active()){
            estop_clear();
            fault_clear();
            timer1_unlock();
            cleared = true;
        }
    }

    if(!cleared){
        usart0_send_string_P(PSTR("WARNING ESTOP still pressed"));
        usart0_send_newline();
        return false;
    }

    usart0_send_string_P(PSTR("OK reset, tasks not resumed"));
    usart0_send_newline();
    return true;
}
