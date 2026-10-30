#include "command/command_servo.h"
#include "command.h"
#include "config.h"
#include "servo_ctrl.h"
#include "usart.h"

#include <string.h>
#include <avr/pgmspace.h>

static void command_servo_warning(const char *text)
{
    usart0_send_string_P(PSTR("WARNING "));
    usart0_send_string_P(text);
    usart0_send_newline();
}

static bool command_servo_result(servo_cmd_t result, const char *bad)
{
    if(result == SERVO_CMD_OK){
        return true;
    }

    if(result == SERVO_CMD_ESTOP){
        command_servo_warning(PSTR("servo blocked by ESTOP, release and reset first"));
    } else if(result == SERVO_CMD_NO_DATA){
        command_servo_warning(PSTR("servo pot needs fresh valid ADC data on that channel"));
    } else {
        command_servo_warning(bad);
    }
    return false;
}

void command_servo_execute(char *args[], uint8_t count, uint32_t now)
{
    int32_t value = 0;

    if(count == 3 && strcmp(args[1], "angle") == 0){
        if(!command_parse_int32(args[2], &value) || value < 0 || value > (int32_t)CONFIG_SERVO_ANGLE_MAX){
            command_servo_warning(PSTR("use servo angle <0-180>"));
            return;
        }

        if(command_servo_result(servo_ctrl_angle((uint32_t)value, now), PSTR("use servo angle <0-180>"))){
            usart0_send_string_P(PSTR("OK servo angle "));
            usart0_send_uint32((uint32_t)value);
            usart0_send_string_P(PSTR(", fixed, at "));
            usart0_send_uint32(servo_ctrl_get_speed());
            usart0_send_string_P(PSTR(" deg/s"));
            usart0_send_newline();
        }
    }
    else if(count == 3 && strcmp(args[1], "speed") == 0){
        if(!command_parse_int32(args[2], &value) || value < (int32_t)CONFIG_SERVO_SPEED_MIN || value > (int32_t)CONFIG_SERVO_SPEED_MAX){
            command_servo_warning(PSTR("servo speed must be 1-600 deg/s"));
            return;
        }

        if(command_servo_result(servo_ctrl_speed((uint32_t)value), PSTR("servo speed must be 1-600 deg/s"))){
            usart0_send_string_P(PSTR("OK servo speed "));
            usart0_send_uint32((uint32_t)value);
            usart0_send_string_P(PSTR(" deg/s"));
            usart0_send_newline();
        }
    }
    else if(count == 3 && strcmp(args[1], "pot") == 0){
        if(!command_parse_int32(args[2], &value) || value < 0 || value >= CONFIG_POT_COUNT){
            command_servo_warning(PSTR("use servo pot <0|1>"));
            return;
        }

        if(command_servo_result(servo_ctrl_pot((uint8_t)value, now), PSTR("use servo pot <0|1>"))){
            usart0_send_string_P(PSTR("OK servo pot A"));
            usart0_send_uint32((uint32_t)value);
            usart0_send_string_P(PSTR(", target follows the filtered pot"));
            usart0_send_newline();
        }
    }
    else if(count == 2 && strcmp(args[1], "off") == 0){
        if(servo_ctrl_off()){
            usart0_send_string_P(PSTR("OK servo off, pin low after the current pulse"));
        } else {
            usart0_send_string_P(PSTR("OK servo off, already off"));
        }
        usart0_send_newline();
    }
    else if(count == 2 && strcmp(args[1], "status") == 0){
        servo_ctrl_request_report(now);
    }
    else {
        command_servo_warning(PSTR("use servo angle <0-180> / speed <deg/s> / pot <0|1> / off / status"));
    }
}
