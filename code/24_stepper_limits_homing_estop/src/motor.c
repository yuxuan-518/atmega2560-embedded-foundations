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
static int32_t position = 0;

static void motor_sync_position(void)
{
    uint32_t steps = timer1_take_steps();

    if(motor_get_direction() == MOTOR_DIR_MAX){
        position += steps;
    } else {
        position -= steps;
    }
}

static void motor_flip_direction(void)
{
    motor_sync_position();
    PORTB ^= (1 << PORTB6);
    _delay_us(10);
}

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

static bool motor_launch(void)
{
    if(timer1_is_locked()){
        motor_state = MOTOR_STOPPED;
        return false;
    }

    if(accel_hz_per_s == 0){
        current_hz = target_hz;
    } else {
        current_hz = MOTOR_START_HZ;
    }

    accel_remainder = 0;
    timer1_set_hz(current_hz);

    if(!timer1_start()){
        motor_state = MOTOR_STOPPED;
        return false;
    }

    motor_follow_target();
    return true;
}

static void motor_halt(void)
{
    timer1_stop();
    motor_sync_position();
    current_hz = MOTOR_START_HZ;
    accel_remainder = 0;
    motor_state = MOTOR_STOPPED;
}

void motor_init(void)
{
    PORTB &= ~((1 << PORTB5) | (1 << PORTB6));
    DDRB |= (1 << DDB5) | (1 << DDB6);

    timer1_init(MOTOR_START_HZ);
    timer1_set_count_level(MOTOR_STEP_ACTIVE_LEVEL);
}

void motor_update(uint32_t now)
{
    if(now - last_update < MOTOR_UPDATE_MS){
        return;
    }

    uint32_t elapsed = now - last_update;
    last_update = now;

    motor_sync_position();

    if(motor_state == MOTOR_STOPPED || motor_state == MOTOR_JOG){
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
        motor_halt();
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

bool motor_start(void)
{
    if(motor_state == MOTOR_STOPPED){
        return motor_launch();
    }

    if(motor_state == MOTOR_STOPPING){
        if(reverse_pending){
            motor_state = MOTOR_REVERSING;
        } else {
            motor_follow_target();
        }
    }

    return motor_state != MOTOR_JOG;
}

void motor_stop(void)
{
    if(motor_state == MOTOR_JOG){
        motor_halt();
        return;
    }

    if(motor_state == MOTOR_STOPPED || motor_state == MOTOR_STOPPING){
        return;
    }

    motor_state = MOTOR_STOPPING;
}

void motor_stop_now(void)
{
    reverse_pending = false;
    motor_halt();
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
    } else if(motor_state != MOTOR_JOG){
        reverse_pending = true;
        motor_state = MOTOR_REVERSING;
    }
}

bool motor_jog(uint8_t dir, uint32_t Hz)
{
    if(motor_state != MOTOR_STOPPED || timer1_is_locked()){
        return false;
    }

    motor_set_direction(dir);

    current_hz = Hz;
    if(!timer1_set_hz(Hz)){
        return false;
    }

    if(!timer1_start()){
        return false;
    }

    motor_state = MOTOR_JOG;
    return true;
}

bool motor_set_direction(uint8_t dir)
{
    if(motor_state != MOTOR_STOPPED){
        return false;
    }

    if(motor_get_direction() != dir){
        motor_flip_direction();
    }

    return true;
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

void motor_set_position(int32_t steps)
{
    motor_sync_position();
    position = steps;
}

motor_state_t motor_get_state(void)
{
    return motor_state;
}

bool motor_is_moving(void)
{
    return motor_state != MOTOR_STOPPED;
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
    uint8_t level = (PORTB & (1 << PORTB6)) != 0;

    if(level == MOTOR_DIR_MAX_LEVEL){
        return MOTOR_DIR_MAX;
    }
    return MOTOR_DIR_MIN;
}

int32_t motor_get_position(void)
{
    motor_sync_position();
    return position;
}
