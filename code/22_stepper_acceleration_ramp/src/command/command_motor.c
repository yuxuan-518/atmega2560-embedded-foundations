#include "command/command_motor.h"
#include "command.h"
#include "motor.h"
#include "demo.h"
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
    }
}

void command_motor_send_status(void)
{
    usart0_send_string("STATE: ");
    command_motor_send_state(motor_get_state());
    usart0_send_string(", HZ: ");
    usart0_send_uint32(motor_get_current_hz());
    usart0_send_string(", TARGET: ");
    usart0_send_uint32(motor_get_target_hz());
    usart0_send_string(", ACCEL: ");
    usart0_send_uint32(motor_get_accel());
    usart0_send_string(" Hz/s, DIR: ");
    usart0_send_uint32(motor_get_direction());
    usart0_send_newline();
}

bool command_motor_log_enabled(void)
{
    return log_enabled;
}

static void command_motor_send_help(void)
{
    usart0_send_string("start / stop / reverse / status\r\n");
    usart0_send_string("speed <Hz>      100-5000\r\n");
    usart0_send_string("accel <Hz/s>    0-20000, 0 = direct\r\n");
    usart0_send_string("accel low/high  500 / 2000\r\n");
    usart0_send_string("demo            accel-cruise-decel-stop\r\n");
    usart0_send_string("log on/off      LOG,ms,Hz every 100 ms\r\n");
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
        motor_start();
        usart0_send_string("OK start");
        usart0_send_newline();
    }
    else if(strcmp(line, "stop") == 0){
        motor_stop();
        usart0_send_string("OK stop");
        usart0_send_newline();
    }
    else if(strcmp(line, "reverse") == 0){
        motor_reverse();
        usart0_send_string("OK reverse");
        usart0_send_newline();
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
        demo_start();
    }
    else if(strcmp(line, "log on") == 0){
        log_enabled = true;
        usart0_send_string("OK log on");
        usart0_send_newline();
    }
    else if(strcmp(line, "log off") == 0){
        log_enabled = false;
        usart0_send_string("OK log off");
        usart0_send_newline();
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
