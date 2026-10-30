#include "command/command_motor.h"
#include "command.h"
#include "motor.h"
#include "demo.h"
#include "limit.h"
#include "estop.h"
#include "fault.h"
#include "homing.h"
#include "safety.h"
#include "usart.h"

#include <string.h>

static bool log_enabled = false;

void command_motor_send_state(motor_state_t state)
{
    switch(state){
        case MOTOR_STOPPED:
            usart0_send_string("STOPPED");
            break;
        case MOTOR_ACCEL:
            usart0_send_string("ACCEL");
            break;
        case MOTOR_CRUISE:
            usart0_send_string("CRUISE");
            break;
        case MOTOR_DECEL:
            usart0_send_string("DECEL");
            break;
        case MOTOR_STOPPING:
            usart0_send_string("STOPPING");
            break;
        case MOTOR_REVERSING:
            usart0_send_string("REVERSING");
            break;
        case MOTOR_JOG:
            usart0_send_string("JOG");
            break;
    }
}

static void command_motor_send_dir(uint8_t dir)
{
    if(dir == MOTOR_DIR_MAX){
        usart0_send_string("MAX");
    } else {
        usart0_send_string("MIN");
    }
}

static void command_motor_send_int32(int32_t number)
{
    if(number < 0){
        usart0_send_string("-");
        number = -number;
    }
    usart0_send_uint32(number);
}

void command_motor_send_status(void)
{
    usart0_send_string("STATE: ");
    command_motor_send_state(motor_get_state());
    usart0_send_string(", HZ: ");
    usart0_send_uint32(motor_get_current_hz());
    usart0_send_string("/");
    usart0_send_uint32(motor_get_target_hz());
    usart0_send_string(", ACCEL: ");
    usart0_send_uint32(motor_get_accel());
    usart0_send_string(", DIR: ");
    command_motor_send_dir(motor_get_direction());
    usart0_send_string(", POS: ");
    command_motor_send_int32(motor_get_position());
    usart0_send_newline();

    usart0_send_string("HOME: ");
    homing_send_phase();
    usart0_send_string(", HOMED: ");
    usart0_send_uint32(homing_is_homed());
    usart0_send_string(", LIMIT MIN: ");
    usart0_send_uint32(limit_min_active());
    usart0_send_string(", MAX: ");
    usart0_send_uint32(limit_max_active());
    usart0_send_string(", ESTOP: ");
    usart0_send_uint32(estop_input_active());
    usart0_send_string(", FAULT: ");
    fault_send_name(fault_get());
    usart0_send_newline();
}

bool command_motor_log_enabled(void)
{
    return log_enabled;
}

static void command_motor_send_help(void)
{
    usart0_send_string("start / stop / reverse / status\r\n");
    usart0_send_string("move min / move max\r\n");
    usart0_send_string("speed <Hz>      100-5000\r\n");
    usart0_send_string("accel <Hz/s>    0-20000, 0 = direct\r\n");
    usart0_send_string("accel low/high  500 / 2000\r\n");
    usart0_send_string("home            find MIN switch, set POS 0\r\n");
    usart0_send_string("exit            only way to leave an active limit\r\n");
    usart0_send_string("reset           clear fault\r\n");
    usart0_send_string("demo / log on / log off\r\n");
}

static bool command_motor_check_ready(void)
{
    if(fault_active()){
        usart0_send_string("WARNING FAULT: ");
        fault_send_name(fault_get());
        usart0_send_string(", type reset");
        usart0_send_newline();
        return false;
    }

    if(homing_busy()){
        usart0_send_string("WARNING homing busy, type stop to abort");
        usart0_send_newline();
        return false;
    }

    return true;
}

static bool command_motor_check_limit(uint8_t dir)
{
    if(dir == MOTOR_DIR_MIN && limit_min_active()){
        usart0_send_string("WARNING LIMIT MIN active, use exit");
        usart0_send_newline();
        return false;
    }

    if(dir == MOTOR_DIR_MAX && limit_max_active()){
        usart0_send_string("WARNING LIMIT MAX active, use exit");
        usart0_send_newline();
        return false;
    }

    return true;
}

static bool command_motor_check_limit_clear(void)
{
    if(limit_min_active()){
        usart0_send_string("WARNING LIMIT MIN active, use exit");
        usart0_send_newline();
        return false;
    }

    if(limit_max_active()){
        usart0_send_string("WARNING LIMIT MAX active, use exit");
        usart0_send_newline();
        return false;
    }

    return true;
}

static bool command_motor_check_stopped(void)
{
    if(motor_is_moving()){
        usart0_send_string("WARNING stop motor first");
        usart0_send_newline();
        return false;
    }
    return true;
}

static void command_motor_send_ok(const char text[])
{
    usart0_send_string("OK ");
    usart0_send_string(text);
    usart0_send_newline();
}

static void command_motor_move(uint8_t dir)
{
    if(!command_motor_check_ready() || !command_motor_check_limit_clear()){
        return;
    }

    if(motor_is_moving() && motor_get_direction() != dir){
        usart0_send_string("WARNING moving the other way, stop or reverse first");
        usart0_send_newline();
        return;
    }

    motor_set_direction(dir);

    if(motor_start()){
        command_motor_send_ok("move");
    } else {
        usart0_send_string("WARNING motion locked");
        usart0_send_newline();
    }
}

static void command_motor_set_accel(uint32_t Hz_per_s)
{
    if(motor_set_accel(Hz_per_s)){
        usart0_send_string("OK accel ");
        usart0_send_uint32(Hz_per_s);
        usart0_send_string(" Hz/s");
    } else {
        usart0_send_string("WARNING accel must be 0-");
        usart0_send_uint32(MOTOR_ACCEL_MAX);
        usart0_send_string(" Hz/s");
    }
    usart0_send_newline();
}

void command_motor_execute(const char line[])
{
    if(strcmp(line, "start") == 0){
        command_motor_move(motor_get_direction());
    }
    else if(strcmp(line, "move min") == 0){
        command_motor_move(MOTOR_DIR_MIN);
    }
    else if(strcmp(line, "move max") == 0){
        command_motor_move(MOTOR_DIR_MAX);
    }
    else if(strcmp(line, "stop") == 0){
        if(homing_busy()){
            homing_abort();
        } else {
            motor_stop();
        }
        command_motor_send_ok("stop");
    }
    else if(strcmp(line, "reverse") == 0){
        if(!command_motor_check_ready()){
            return;
        }

        uint8_t new_dir = MOTOR_DIR_MAX;
        if(motor_get_direction() == MOTOR_DIR_MAX){
            new_dir = MOTOR_DIR_MIN;
        }

        if(!command_motor_check_limit(new_dir)){
            return;
        }

        motor_reverse();
        command_motor_send_ok("reverse");
    }
    else if(strcmp(line, "home") == 0){
        if(!command_motor_check_ready() || !command_motor_check_stopped()){
            return;
        }

        homing_start();
        command_motor_send_ok("home");
    }
    else if(strcmp(line, "exit") == 0){
        if(!command_motor_check_ready() || !command_motor_check_stopped()){
            return;
        }

        homing_exit();
        command_motor_send_ok("exit");
    }
    else if(strcmp(line, "reset") == 0){
        safety_reset();
    }
    else if(strncmp(line, "speed ", 6) == 0){
        uint32_t Hz = command_parse_uint32(line + 6);

        if(motor_set_target(Hz)){
            usart0_send_string("OK target ");
            usart0_send_uint32(Hz);
            usart0_send_string(" Hz");
        } else {
            usart0_send_string("WARNING speed must be ");
            usart0_send_uint32(MOTOR_MIN_HZ);
            usart0_send_string("-");
            usart0_send_uint32(MOTOR_MAX_HZ);
            usart0_send_string(" Hz");
        }
        usart0_send_newline();
    }
    else if(strcmp(line, "accel low") == 0){
        command_motor_set_accel(MOTOR_ACCEL_LOW);
    }
    else if(strcmp(line, "accel high") == 0){
        command_motor_set_accel(MOTOR_ACCEL_HIGH);
    }
    else if(strcmp(line, "accel 0") == 0){
        command_motor_set_accel(0);
    }
    else if(strncmp(line, "accel ", 6) == 0){
        uint32_t Hz_per_s = command_parse_uint32(line + 6);

        if(Hz_per_s == 0){
            usart0_send_string("WARNING accel must be a number");
            usart0_send_newline();
        } else {
            command_motor_set_accel(Hz_per_s);
        }
    }
    else if(strcmp(line, "demo") == 0){
        if(!command_motor_check_ready() || !command_motor_check_limit_clear()){
            return;
        }
        demo_start();
    }
    else if(strcmp(line, "log on") == 0){
        log_enabled = true;
        command_motor_send_ok("log on");
    }
    else if(strcmp(line, "log off") == 0){
        log_enabled = false;
        command_motor_send_ok("log off");
    }
    else if(strcmp(line, "status") == 0){
        command_motor_send_status();
    }
    else if(strcmp(line, "help") == 0){
        command_motor_send_help();
    }
    else {
        usart0_send_string("WARNING unknown command, type help");
        usart0_send_newline();
    }
}
