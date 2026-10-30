#include "motor.h"
#include "timer.h"

#include <avr/io.h>
#include <util/delay.h>

static motor_state_t motor_state = MOTOR_STOPPED;
static uint32_t current_hz = MOTOR_START_HZ;
static uint32_t target_hz = MOTOR_DEFAULT_TARGET_HZ;
static uint32_t accel_hz_per_s = MOTOR_ACCEL_LOW;
static uint32_t accel_remainder = 0;
static uint32_t last_update = 0;
static bool reverse_pending = false;

static void motor_follow_target(void)
{
    if(current_hz < target_hz){
        motor_state = MOTOR_ACCEL;
    } else if(current_hz > target_hz){
        motor_state = MOTOR_DECEL;
    } else {
        motor_state = MOTOR_CRUISE;
    }
}

static void motor_flip_direction(void)
{
    PORTB ^= (1 << PORTB6);
    _delay_us(10);
}

static void motor_launch(void)
{
    if(accel_hz_per_s == 0){
        current_hz = target_hz;
    } else {
        current_hz = MOTOR_START_HZ;
    }

    accel_remainder = 0;
    timer1_set_hz(current_hz);
    timer1_start();
    motor_follow_target();
}

static void motor_halt(void)
{
    timer1_stop();
    current_hz = MOTOR_START_HZ;
    accel_remainder = 0;
    motor_state = MOTOR_STOPPED;
}

void motor_init(void)
{
    PORTB &= ~((1 << PORTB5) | (1 << PORTB6));
    DDRB |= (1 << DDB5) | (1 << DDB6);

    timer1_init(MOTOR_START_HZ);
}

void motor_update(uint32_t now)
{
    if(now - last_update < MOTOR_UPDATE_MS){
        return;
    }

    uint32_t elapsed = now - last_update;
    last_update = now;

    if(motor_state == MOTOR_STOPPED){
        return;
    }

    uint32_t goal = target_hz;
    if(motor_state == MOTOR_STOPPING || motor_state == MOTOR_REVERSING){
        goal = MOTOR_START_HZ;
    }

    uint32_t last_hz = current_hz;

    if(accel_hz_per_s == 0){
        current_hz = goal;
    } else {
        accel_remainder += accel_hz_per_s * elapsed;
        uint32_t delta = accel_remainder / 1000;
        accel_remainder %= 1000;

        if(current_hz < goal){
            if(goal - current_hz <= delta){
                current_hz = goal;
            } else {
                current_hz += delta;
            }
        } else if(current_hz > goal){
            if(current_hz - goal <= delta){
                current_hz = goal;
            } else {
                current_hz -= delta;
            }
        }
    }

    if(current_hz != goal){
        if(current_hz != last_hz){
            timer1_update_hz(current_hz);
        }
        return;
    }

    if(motor_state == MOTOR_STOPPING){
        motor_halt();

        if(reverse_pending){
            motor_flip_direction();
            reverse_pending = false;
        }
    } else if(motor_state == MOTOR_REVERSING){
        timer1_stop();
        motor_flip_direction();
        reverse_pending = false;
        motor_launch();
    } else {
        if(current_hz != last_hz){
            timer1_update_hz(current_hz);
        }
        motor_state = MOTOR_CRUISE;
    }
}

void motor_start(void)
{
    if(motor_state == MOTOR_STOPPED){
        motor_launch();
    } else if(motor_state == MOTOR_STOPPING){
        if(reverse_pending){
            motor_state = MOTOR_REVERSING;
        } else {
            motor_follow_target();
        }
    }
}

void motor_stop(void)
{
    if(motor_state == MOTOR_STOPPED || motor_state == MOTOR_STOPPING){
        return;
    }

    motor_state = MOTOR_STOPPING;
}

void motor_reverse(void)
{
    if(motor_state == MOTOR_STOPPED){
        motor_flip_direction();
    } else if(motor_state == MOTOR_STOPPING){
        reverse_pending = !reverse_pending;
    } else if(motor_state == MOTOR_REVERSING){
        reverse_pending = false;
        motor_follow_target();
    } else {
        reverse_pending = true;
        motor_state = MOTOR_REVERSING;
    }
}

bool motor_set_target(uint32_t Hz)
{
    if(Hz < MOTOR_MIN_HZ || Hz > MOTOR_MAX_HZ){
        return false;
    }

    target_hz = Hz;

    if(motor_state == MOTOR_ACCEL || motor_state == MOTOR_CRUISE || motor_state == MOTOR_DECEL){
        motor_follow_target();
    }

    return true;
}

bool motor_set_accel(uint32_t Hz_per_s)
{
    if(Hz_per_s > MOTOR_ACCEL_MAX){
        return false;
    }

    accel_hz_per_s = Hz_per_s;
    accel_remainder = 0;
    return true;
}

motor_state_t motor_get_state(void)
{
    return motor_state;
}

uint32_t motor_get_current_hz(void)
{
    if(motor_state == MOTOR_STOPPED){
        return 0;
    }
    return current_hz;
}

uint32_t motor_get_target_hz(void)
{
    return target_hz;
}

uint32_t motor_get_accel(void)
{
    return accel_hz_per_s;
}

uint8_t motor_get_direction(void)
{
    return (PORTB & (1 << PORTB6)) != 0;
}
