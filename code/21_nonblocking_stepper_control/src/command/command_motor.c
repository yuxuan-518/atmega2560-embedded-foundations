#include "command/command_motor.h"
#include "command/command.h"
#include "motor.h"
#include "usart.h"

#include <string.h>

void command_motor_send_status(void)
{
    usart0_send_string("RUN: ");
    if(motor_is_running()){
        usart0_send_string("ON");
    } else {
        usart0_send_string("OFF");
    }

    usart0_send_string(", SPEED: ");
    usart0_send_uint32(motor_get_speed());
    usart0_send_string(" Hz, DIR: ");
    usart0_send_uint32(motor_get_direction());
    usart0_send_newline();
}

static void command_motor_send_help(void)
{
    usart0_send_string("      command      control\r\n"
                       "        start      start motor\r\n"
                       "         stop      stop motor\r\n"
                       "  speed <Hz>       set speed (50-1000)\r\n"
                       "      reverse      change direction\r\n"
                       "       status      show motor status\r\n");
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
    else if(strncmp(line, "speed ", 6) == 0){
        uint32_t Hz = command_parse_uint32(line + 6);

        if(motor_set_speed(Hz)){
            usart0_send_string("OK speed ");
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
    else if(strcmp(line, "reverse") == 0){
        motor_change_direction();
        usart0_send_string("OK reverse, DIR: ");
        usart0_send_uint32(motor_get_direction());
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
