#include "homing.h"
#include "motor.h"
#include "limit.h"
#include "fault.h"
#include "usart.h"

typedef enum {
    HOME_IDLE,
    HOME_REQUEST,
    HOME_LEAVE,
    HOME_SEEK_FAST,
    HOME_BACKOFF,
    HOME_SEEK_SLOW,
    EXIT_REQUEST,
    EXIT_MOVE
} home_phase_t;

static home_phase_t home_phase = HOME_IDLE;
static bool homed = false;
static bool exit_from_min = true;
static bool switch_released = false;
static uint32_t phase_start_ms = 0;
static int32_t phase_start_pos = 0;
static int32_t release_pos = 0;

static uint32_t homing_distance(int32_t from, int32_t to)
{
    int32_t distance = to - from;

    if(distance < 0){
        distance = -distance;
    }

    return distance;
}

static void homing_send_time(uint32_t now)
{
    usart0_send_string("[");
    usart0_send_uint32(now);
    usart0_send_string(" ms] ");
}

static void homing_fail(fault_t fault, uint32_t now)
{
    motor_stop_now();
    home_phase = HOME_IDLE;
    homed = false;
    fault_set(fault, now);
}

static void homing_enter(home_phase_t phase, uint32_t now, uint8_t dir, uint32_t Hz)
{
    motor_stop_now();

    home_phase = phase;
    phase_start_ms = now;
    phase_start_pos = motor_get_position();
    switch_released = false;

    homing_send_time(now);
    usart0_send_string("HOME: ");
    homing_send_phase();
    usart0_send_newline();

    if(!motor_jog(dir, Hz)){
        homing_fail(FAULT_MOTION_STOPPED, now);
    }
}

void homing_start(void)
{
    homed = false;
    home_phase = HOME_REQUEST;
}

void homing_exit(void)
{
    home_phase = EXIT_REQUEST;
}

void homing_abort(void)
{
    if(home_phase == HOME_IDLE){
        return;
    }

    motor_stop_now();

    if(home_phase != EXIT_REQUEST && home_phase != EXIT_MOVE){
        homed = false;
    }

    home_phase = HOME_IDLE;

    usart0_send_string("HOME ABORT");
    usart0_send_newline();
}

void homing_invalidate(void)
{
    homed = false;
}

void homing_update(uint32_t now)
{
    if(home_phase == HOME_IDLE){
        return;
    }

    if(home_phase == HOME_REQUEST){
        if(limit_min_active()){
            homing_enter(HOME_LEAVE, now, MOTOR_DIR_MAX, HOME_EXIT_HZ);
        } else {
            homing_enter(HOME_SEEK_FAST, now, MOTOR_DIR_MIN, HOME_FAST_HZ);
        }
        return;
    }

    if(home_phase == EXIT_REQUEST){
        if(limit_min_active()){
            exit_from_min = true;
            homing_enter(EXIT_MOVE, now, MOTOR_DIR_MAX, HOME_EXIT_HZ);
        } else if(limit_max_active()){
            exit_from_min = false;
            homing_enter(EXIT_MOVE, now, MOTOR_DIR_MIN, HOME_EXIT_HZ);
        } else {
            home_phase = HOME_IDLE;
            usart0_send_string("EXIT: no limit active");
            usart0_send_newline();
        }
        return;
    }

    if(home_phase != EXIT_MOVE && limit_max_active()){
        homing_fail(FAULT_HOME_WRONG_LIMIT, now);
        return;
    }

    int32_t pos = motor_get_position();
    uint32_t steps = homing_distance(phase_start_pos, pos);
    uint32_t elapsed = now - phase_start_ms;

    if(home_phase == HOME_SEEK_FAST || home_phase == HOME_SEEK_SLOW){
        if(limit_min_active()){
            motor_stop_now();

            if(home_phase == HOME_SEEK_FAST){
                homing_enter(HOME_BACKOFF, now, MOTOR_DIR_MAX, HOME_EXIT_HZ);
            } else {
                motor_set_position(0);
                homed = true;
                home_phase = HOME_IDLE;

                homing_send_time(now);
                usart0_send_string("HOME DONE, POS: 0");
                usart0_send_newline();
            }
            return;
        }

        uint32_t max_steps = HOME_SEEK_MAX_STEPS;
        uint32_t timeout = HOME_SEEK_TIMEOUT_MS;

        if(home_phase == HOME_SEEK_SLOW){
            max_steps = HOME_SLOW_MAX_STEPS;
            timeout = HOME_SLOW_TIMEOUT_MS;
        }

        if(steps > max_steps || elapsed > timeout){
            homing_fail(FAULT_HOME_NOT_FOUND, now);
            return;
        }
    } else {
        bool switch_active = limit_min_active();

        if(home_phase == EXIT_MOVE && !exit_from_min){
            switch_active = limit_max_active();
        }

        if(!switch_released){
            if(!switch_active){
                switch_released = true;
                release_pos = pos;
            } else if(steps > HOME_RELEASE_MAX_STEPS || elapsed > HOME_RELEASE_TIMEOUT_MS){
                homing_fail(FAULT_SWITCH_STUCK, now);
                return;
            }
        } else if(homing_distance(release_pos, pos) >= HOME_BACKOFF_EXTRA_STEPS){
            motor_stop_now();

            if(home_phase == HOME_LEAVE){
                homing_enter(HOME_SEEK_FAST, now, MOTOR_DIR_MIN, HOME_FAST_HZ);
            } else if(home_phase == HOME_BACKOFF){
                homing_enter(HOME_SEEK_SLOW, now, MOTOR_DIR_MIN, HOME_SLOW_HZ);
            } else {
                home_phase = HOME_IDLE;

                homing_send_time(now);
                usart0_send_string("EXIT DONE");
                usart0_send_newline();
            }
            return;
        }
    }

    if(motor_get_state() == MOTOR_STOPPED){
        homing_fail(FAULT_MOTION_STOPPED, now);
    }
}

bool homing_busy(void)
{
    return home_phase != HOME_IDLE;
}

bool homing_is_homed(void)
{
    return homed;
}

void homing_send_phase(void)
{
    switch(home_phase){
        case HOME_IDLE:
            usart0_send_string("IDLE");
            break;
        case HOME_REQUEST:
            usart0_send_string("REQUEST");
            break;
        case HOME_LEAVE:
            usart0_send_string("LEAVE");
            break;
        case HOME_SEEK_FAST:
            usart0_send_string("SEEK_FAST");
            break;
        case HOME_BACKOFF:
            usart0_send_string("BACKOFF");
            break;
        case HOME_SEEK_SLOW:
            usart0_send_string("SEEK_SLOW");
            break;
        case EXIT_REQUEST:
            usart0_send_string("EXIT_REQUEST");
            break;
        case EXIT_MOVE:
            usart0_send_string("EXIT");
            break;
    }
}
