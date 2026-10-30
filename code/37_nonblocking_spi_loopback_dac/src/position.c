#include "position.h"
#include "config.h"
#include "motor.h"
#include "homing.h"
#include "limit.h"
#include "fault.h"
#include "usart.h"
#include <avr/pgmspace.h>

static bool busy = false;
static bool report = false;
static bool last_ok = false;
static int32_t target = 0;

static void position_warning(const char *text)
{
    usart0_send_string_P(PSTR("WARNING "));
    usart0_send_string_P(text);
    usart0_send_newline();
}

static void position_send_int32(int32_t number)
{
    if(number < 0){
        usart0_send_string_P(PSTR("-"));
        number = -number;
    }
    usart0_send_uint32(number);
}

static bool position_check_ready(void)
{
    if(fault_active()){
        position_warning(PSTR("fault active, type reset"));
        return false;
    }

    if(busy || motor_is_moving()){
        position_warning(PSTR("motor busy, stop first"));
        return false;
    }

    if(limit_any_active()){
        position_warning(PSTR("limit active, use exit"));
        return false;
    }

    int32_t pos = motor_get_position();

    if(pos > CONFIG_POSITION_ABS_MAX || pos < -CONFIG_POSITION_ABS_MAX){
        position_warning(PSTR("position out of range, home again"));
        return false;
    }

    return true;
}

static bool position_start(int32_t new_target, bool new_report)
{
    int32_t steps = new_target - motor_get_position();

    target = new_target;
    report = new_report;

    if(steps == 0){
        busy = false;
        last_ok = true;

        if(report){
            usart0_send_string_P(PSTR("OK already at "));
            position_send_int32(target);
            usart0_send_newline();
        }
        return true;
    }

    if(steps > CONFIG_MOVE_MAX_STEPS || steps < -CONFIG_MOVE_MAX_STEPS){
        position_warning(PSTR("move too far, max 1000000 steps"));
        return false;
    }

    if(!motor_move(steps)){
        position_warning(PSTR("motion locked"));
        return false;
    }

    busy = true;
    last_ok = false;
    return true;
}

bool position_check_target(int32_t new_target)
{
    const config_t *cfg = config_get();

    if(!homing_is_homed()){
        return true;
    }

    if(new_target < cfg->soft_min || new_target > cfg->soft_max){
        usart0_send_string_P(PSTR("WARNING target outside soft limit "));
        position_send_int32(cfg->soft_min);
        usart0_send_string_P(PSTR(".."));
        position_send_int32(cfg->soft_max);
        usart0_send_newline();
        return false;
    }

    return true;
}

bool position_move_steps(int32_t steps, bool new_report)
{
    if(!position_check_ready()){
        return false;
    }

    if(steps > CONFIG_MOVE_MAX_STEPS || steps < -CONFIG_MOVE_MAX_STEPS){
        position_warning(PSTR("steps must be -1000000..1000000"));
        return false;
    }

    int32_t new_target = motor_get_position() + steps;

    if(!position_check_target(new_target)){
        return false;
    }

    return position_start(new_target, new_report);
}

bool position_goto(int32_t new_target, bool new_report)
{
    if(!homing_is_homed()){
        position_warning(PSTR("not homed, type home first"));
        return false;
    }

    if(!position_check_ready() || !position_check_target(new_target)){
        return false;
    }

    return position_start(new_target, new_report);
}

void position_update(uint32_t now)
{
    if(!busy || motor_is_moving()){
        return;
    }

    busy = false;

    int32_t pos = motor_get_position();
    last_ok = (pos == target);

    if(report){
        usart0_send_string_P(PSTR("["));
        usart0_send_uint32(now);
        usart0_send_string_P(PSTR(" ms] "));

        if(last_ok){
            usart0_send_string_P(PSTR("MOVE DONE"));
        } else {
            usart0_send_string_P(PSTR("MOVE ABORT"));
        }

        usart0_send_string_P(PSTR(", TARGET: "));
        position_send_int32(target);
        usart0_send_string_P(PSTR(", POS: "));
        position_send_int32(pos);
        usart0_send_newline();
    }
}

void position_abort(void)
{
    if(busy){
        motor_stop_now();
    }
}

bool position_busy(void)
{
    return busy;
}

bool position_last_ok(void)
{
    return last_ok;
}

int32_t position_get_target(void)
{
    return target;
}

uint32_t position_get_remaining(void)
{
    if(!busy){
        return 0;
    }
    return motor_get_remaining();
}
