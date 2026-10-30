#include "repeat.h"
#include "config.h"
#include "position.h"
#include "homing.h"
#include "motor.h"
#include "usart.h"
#include <avr/pgmspace.h>

static bool active = false;
static bool leg_running = false;
static int32_t point_a = 0;
static int32_t point_b = 0;
static uint32_t total_legs = 0;
static uint32_t leg_index = 0;
static uint32_t legs_ok = 0;

static void repeat_send_int32(int32_t number)
{
    if(number < 0){
        usart0_send_string_P(PSTR("-"));
        number = -number;
    }
    usart0_send_uint32(number);
}

static void repeat_send_summary(const char *text)
{
    usart0_send_string_P(text);
    usart0_send_string_P(PSTR(", OK legs: "));
    usart0_send_uint32(legs_ok);
    usart0_send_string_P(PSTR("/"));
    usart0_send_uint32(total_legs);
    usart0_send_newline();
}

bool repeat_start(int32_t a, int32_t b, int32_t cycles)
{
    if(!homing_is_homed()){
        usart0_send_string_P(PSTR("WARNING not homed, type home first"));
        usart0_send_newline();
        return false;
    }

    if(cycles < 1 || cycles > CONFIG_REPEAT_MAX){
        usart0_send_string_P(PSTR("WARNING cycles must be 1-1000"));
        usart0_send_newline();
        return false;
    }

    if(a == b){
        usart0_send_string_P(PSTR("WARNING points must be different"));
        usart0_send_newline();
        return false;
    }

    if(!position_check_target(a) || !position_check_target(b)){
        return false;
    }

    point_a = a;
    point_b = b;
    total_legs = 2UL * cycles;
    leg_index = 0;
    legs_ok = 0;
    leg_running = false;
    active = true;

    usart0_send_string_P(PSTR("REPEAT START"));
    usart0_send_newline();
    return true;
}

void repeat_update(uint32_t now)
{
    if(!active){
        return;
    }

    if(leg_running){
        if(position_busy()){
            return;
        }

        leg_running = false;

        bool ok = position_last_ok();

        usart0_send_string_P(PSTR("["));
        usart0_send_uint32(now);
        usart0_send_string_P(PSTR(" ms] REP "));
        usart0_send_uint32(leg_index + 1);
        usart0_send_string_P(PSTR("/"));
        usart0_send_uint32(total_legs);
        usart0_send_string_P(PSTR(", TARGET: "));
        repeat_send_int32(position_get_target());
        usart0_send_string_P(PSTR(", POS: "));
        repeat_send_int32(motor_get_position());

        if(ok){
            usart0_send_string_P(PSTR(", OK"));
        } else {
            usart0_send_string_P(PSTR(", FAIL"));
        }
        usart0_send_newline();

        if(!ok){
            active = false;
            repeat_send_summary(PSTR("REPEAT ABORT"));
            return;
        }

        legs_ok++;
        leg_index++;

        if(leg_index >= total_legs){
            active = false;
            repeat_send_summary(PSTR("REPEAT DONE"));
            return;
        }
    }

    int32_t next_target = point_a;
    if(leg_index % 2 == 1){
        next_target = point_b;
    }

    if(!position_goto(next_target, false)){
        active = false;
        repeat_send_summary(PSTR("REPEAT ABORT"));
        return;
    }

    leg_running = true;
}

void repeat_abort(void)
{
    if(!active){
        return;
    }

    active = false;
    leg_running = false;
    repeat_send_summary(PSTR("REPEAT ABORT"));
}

bool repeat_busy(void)
{
    return active;
}
