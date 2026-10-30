#include "command/command_motor.h"
#include "command.h"
#include "config.h"
#include "motor.h"
#include "demo.h"
#include "limit.h"
#include "estop.h"
#include "fault.h"
#include "homing.h"
#include "position.h"
#include "repeat.h"
#include "safety.h"
#include "task.h"
#include "report.h"
#include "command/command_nh3.h"
#include "command/command_led.h"
#include "command/command_adc.h"
#include "usart.h"

#include <string.h>

#define COMMAND_MAX_ARGS 4

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

static void command_motor_send_int32(int32_t number)
{
    if(number < 0){
        usart0_send_string("-");
        number = -number;
    }
    usart0_send_uint32(number);
}

static void command_motor_send_dir(uint8_t dir)
{
    if(dir == MOTOR_DIR_MAX){
        usart0_send_string("MAX");
    } else {
        usart0_send_string("MIN");
    }
}

void command_motor_send_status(void)
{
    usart0_send_string("STATE: ");
    command_motor_send_state(motor_get_state());
    usart0_send_string(", TASK: ");
    task_send_name();
    usart0_send_string(", POS: ");
    command_motor_send_int32(motor_get_position());
    usart0_send_string(", TARGET: ");

    if(position_busy()){
        command_motor_send_int32(position_get_target());
    } else {
        usart0_send_string("-");
    }

    usart0_send_string(", REMAIN: ");
    usart0_send_uint32(position_get_remaining());
    usart0_send_string(", HZ: ");
    usart0_send_uint32(motor_get_current_hz());
    usart0_send_string("/");
    usart0_send_uint32(config_get()->speed_hz);
    usart0_send_newline();

    usart0_send_string("HOMED: ");
    usart0_send_uint32(homing_is_homed());
    usart0_send_string(", DIR: ");
    command_motor_send_dir(motor_get_direction());
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

static void command_motor_warning(const char text[])
{
    usart0_send_string("WARNING ");
    usart0_send_string(text);
    usart0_send_newline();
}

static void command_motor_send_ok(const char text[])
{
    usart0_send_string("OK ");
    usart0_send_string(text);
    usart0_send_newline();
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

    if(task_busy()){
        usart0_send_string("WARNING busy: ");
        task_send_name();
        usart0_send_string(", type stop first");
        usart0_send_newline();
        return false;
    }

    return true;
}

static bool command_motor_check_limit_clear(void)
{
    if(limit_min_active()){
        command_motor_warning("LIMIT MIN active, use exit");
        return false;
    }

    if(limit_max_active()){
        command_motor_warning("LIMIT MAX active, use exit");
        return false;
    }

    return true;
}

static bool command_motor_get_int32(char *args[], uint8_t count, uint8_t index, int32_t *value)
{
    if(index >= count || !command_parse_int32(args[index], value)){
        command_motor_warning("bad number, type help");
        return false;
    }
    return true;
}

static void command_motor_run(uint8_t dir)
{
    if(!command_motor_check_ready() || !command_motor_check_limit_clear()){
        return;
    }

    if(homing_is_homed()){
        const config_t *cfg = config_get();
        int32_t target = cfg->soft_min;

        if(dir == MOTOR_DIR_MAX){
            target = cfg->soft_max;
        }

        if(position_goto(target, true)){
            command_motor_send_ok("run to soft limit");
        }
        return;
    }

    motor_set_direction(dir);

    if(motor_start()){
        command_motor_send_ok("run");
    } else {
        command_motor_warning("motion locked");
    }
}

static void command_motor_stop(void)
{
    repeat_abort();
    demo_abort();

    if(homing_busy()){
        homing_abort();
    } else {
        motor_stop();
    }

    command_motor_send_ok("stop");
}

static void command_motor_reverse(void)
{
    if(fault_active() || position_busy() || repeat_busy() || homing_busy() || demo_is_running()){
        command_motor_warning("reverse only for manual run");
        return;
    }

    uint8_t new_dir = MOTOR_DIR_MAX;
    if(motor_get_direction() == MOTOR_DIR_MAX){
        new_dir = MOTOR_DIR_MIN;
    }

    if((new_dir == MOTOR_DIR_MIN && limit_min_active()) ||
       (new_dir == MOTOR_DIR_MAX && limit_max_active())){
        command_motor_warning("limit active on that side, use exit");
        return;
    }

    motor_reverse();
    command_motor_send_ok("reverse");
}

static void command_motor_set(char *args[], uint8_t count)
{
    int32_t value = 0;

    if(count != 3 || !command_motor_get_int32(args, count, 2, &value)){
        command_motor_warning("use set start|max|softmin|softmax <v>");
        return;
    }

    if(task_busy()){
        command_motor_warning("stop motor before set");
        return;
    }

    config_item_t item;

    if(strcmp(args[1], "start") == 0){
        item = CONFIG_START;
    } else if(strcmp(args[1], "max") == 0){
        item = CONFIG_MAX;
    } else if(strcmp(args[1], "softmin") == 0){
        item = CONFIG_SOFT_MIN;
    } else if(strcmp(args[1], "softmax") == 0){
        item = CONFIG_SOFT_MAX;
    } else {
        command_motor_warning("unknown setting");
        return;
    }

    if(config_set(item, value)){
        command_motor_send_ok("set");
    }
}

static void command_motor_accel(char *args[], uint8_t count)
{
    int32_t value = 0;

    if(count != 2){
        command_motor_warning("use accel <Hz/s>|low|high");
        return;
    }

    if(task_busy()){
        command_motor_warning("stop motor before accel");
        return;
    }

    if(strcmp(args[1], "low") == 0){
        value = CONFIG_ACCEL_LOW;
    } else if(strcmp(args[1], "high") == 0){
        value = CONFIG_ACCEL_HIGH;
    } else if(!command_motor_get_int32(args, count, 1, &value)){
        return;
    }

    if(config_set(CONFIG_ACCEL, value)){
        usart0_send_string("OK accel ");
        usart0_send_uint32(value);
        usart0_send_string(" Hz/s");
        usart0_send_newline();
    }
}

void command_motor_execute(const char line[], uint32_t now)
{
    char buffer[COMMAND_LEN];
    char *args[COMMAND_MAX_ARGS];

    strncpy(buffer, line, COMMAND_LEN - 1);
    buffer[COMMAND_LEN - 1] = '\0';

    uint8_t count = command_split(buffer, args, COMMAND_MAX_ARGS);

    if(count == 0){
        return;
    }

    if(count > COMMAND_MAX_ARGS){
        command_motor_warning("too many arguments");
        return;
    }

    int32_t value = 0;

    if(strcmp(args[0], "nh3") == 0){
        command_nh3_execute(args, count);
    }
    else if(strcmp(args[0], "led") == 0){
        command_led_execute(args, count, now);
    }
    else if(strcmp(args[0], "adc") == 0){
        command_adc_execute(args, count);
    }
    else if(strcmp(args[0], "move") == 0){
        if(count != 2 || !command_motor_get_int32(args, count, 1, &value)){
            return;
        }

        if(value == 0){
            command_motor_send_ok("move 0, no motion");
            return;
        }

        if(command_motor_check_ready() && position_move_steps(value, true)){
            command_motor_send_ok("move");
        }
    }
    else if(strcmp(args[0], "goto") == 0){
        if(count != 2 || !command_motor_get_int32(args, count, 1, &value)){
            return;
        }

        if(command_motor_check_ready() && position_goto(value, true)){
            command_motor_send_ok("goto");
        }
    }
    else if(strcmp(args[0], "repeat") == 0){
        int32_t a = 0;
        int32_t b = 0;
        int32_t cycles = 0;

        if(count != 4 ||
           !command_motor_get_int32(args, count, 1, &a) ||
           !command_motor_get_int32(args, count, 2, &b) ||
           !command_motor_get_int32(args, count, 3, &cycles)){
            command_motor_warning("use repeat <a> <b> <cycles>");
            return;
        }

        if(command_motor_check_ready() && command_motor_check_limit_clear()){
            repeat_start(a, b, cycles);
        }
    }
    else if(strcmp(args[0], "start") == 0 && count == 1){
        command_motor_run(motor_get_direction());
    }
    else if(strcmp(args[0], "run") == 0 && count == 2 && strcmp(args[1], "min") == 0){
        command_motor_run(MOTOR_DIR_MIN);
    }
    else if(strcmp(args[0], "run") == 0 && count == 2 && strcmp(args[1], "max") == 0){
        command_motor_run(MOTOR_DIR_MAX);
    }
    else if(strcmp(args[0], "stop") == 0 && count == 1){
        command_motor_stop();
    }
    else if(strcmp(args[0], "reverse") == 0 && count == 1){
        command_motor_reverse();
    }
    else if(strcmp(args[0], "home") == 0 && count == 1){
        if(command_motor_check_ready()){
            homing_start();
            command_motor_send_ok("home");
        }
    }
    else if(strcmp(args[0], "exit") == 0 && count == 1){
        if(command_motor_check_ready()){
            homing_exit();
            command_motor_send_ok("exit");
        }
    }
    else if(strcmp(args[0], "reset") == 0 && count == 1){
        safety_reset();
    }
    else if(strcmp(args[0], "speed") == 0){
        if(count != 2 || !command_motor_get_int32(args, count, 1, &value)){
            return;
        }

        if(config_set(CONFIG_SPEED, value)){
            motor_refresh_target();
            usart0_send_string("OK speed ");
            usart0_send_uint32(value);
            usart0_send_string(" Hz");
            usart0_send_newline();
        }
    }
    else if(strcmp(args[0], "accel") == 0){
        command_motor_accel(args, count);
    }
    else if(strcmp(args[0], "set") == 0){
        command_motor_set(args, count);
    }
    else if(strcmp(args[0], "config") == 0 && count == 1){
        report_request_config();
    }
    else if(strcmp(args[0], "demo") == 0 && count == 1){
        if(!command_motor_check_ready() || !command_motor_check_limit_clear()){
            return;
        }

        if(homing_is_homed()){
            command_motor_warning("homed: use goto or repeat");
            return;
        }

        demo_start();
    }
    else if(strcmp(args[0], "log") == 0 && count == 2 && strcmp(args[1], "on") == 0){
        report_set_log(true);
        command_motor_send_ok("log on");
    }
    else if(strcmp(args[0], "log") == 0 && count == 2 && strcmp(args[1], "off") == 0){
        report_set_log(false);
        command_motor_send_ok("log off");
    }
    else if(strcmp(args[0], "status") == 0 && count == 1){
        report_request_status();
    }
    else if(strcmp(args[0], "help") == 0 && count == 1){
        report_request_help();
    }
    else {
        command_motor_warning("unknown command, type help");
    }
}
