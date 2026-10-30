#include "safety.h"
#include "limit.h"
#include "estop.h"
#include "fault.h"
#include "motor.h"
#include "homing.h"
#include "timer.h"
#include "usart.h"

#include <avr/interrupt.h>
#include <util/atomic.h>

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

static void safety_stop_all(void)
{
    motor_stop_now();
    homing_abort();
    homing_invalidate();
}

static void safety_send_limit_hit(uint32_t now, const char text[])
{
    usart0_send_string("[");
    usart0_send_uint32(now);
    usart0_send_string(" ms] ");
    usart0_send_string(text);
    usart0_send_newline();
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

    limit_update(now);

    if(estop_is_latched() || estop_input_active()){
        estop_latch();

        if(motor_is_moving() || homing_busy()){
            safety_stop_all();
        }
        homing_invalidate();

        fault_set(FAULT_ESTOP, now);
        return;
    }

    if(limit_min_active() && limit_max_active()){
        if(motor_is_moving() || homing_busy()){
            safety_stop_all();
        }
        homing_invalidate();

        fault_set(FAULT_LIMIT_WIRING, now);
        return;
    }

    if(motor_is_moving()){
        uint8_t dir = motor_get_direction();

        if(dir == MOTOR_DIR_MIN && (limit_min_active() || (hit & GUARD_MIN))){
            motor_stop_now();
            safety_send_limit_hit(now, "LIMIT MIN HIT");

            if(!homing_busy()){
                homing_invalidate();
            }
        } else if(dir == MOTOR_DIR_MAX && (limit_max_active() || (hit & GUARD_MAX))){
            motor_stop_now();
            safety_send_limit_hit(now, "LIMIT MAX HIT");

            if(!homing_busy()){
                homing_invalidate();
            }
        }
    }
}

bool safety_reset(void)
{
    if(limit_min_active() && limit_max_active()){
        usart0_send_string("WARNING both limits active, check wiring");
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
        usart0_send_string("WARNING ESTOP still pressed");
        usart0_send_newline();
        return false;
    }

    usart0_send_string("OK reset");
    usart0_send_newline();
    return true;
}
