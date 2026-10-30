#include "command/command_imu.h"
#include "imu.h"
#include "usart.h"

#include <string.h>
#include <avr/pgmspace.h>

static void command_imu_ok(const char *text)
{
    usart0_send_string_P(PSTR("OK imu "));
    usart0_send_string_P(text);
    usart0_send_newline();
}

static void command_imu_warning(const char *text)
{
    usart0_send_string_P(PSTR("WARNING "));
    usart0_send_string_P(text);
    usart0_send_newline();
}

void command_imu_execute(char *args[], uint8_t count, uint32_t now)
{
    if(count == 2 && strcmp(args[1], "start") == 0){
        if(imu_start(now)){
            command_imu_ok(PSTR("start, init then sample every 100 ms"));
        } else {
            command_imu_ok(PSTR("already running"));
        }
    }
    else if(count == 2 && strcmp(args[1], "stop") == 0){
        if(imu_stop(now)){
            command_imu_ok(PSTR("stop"));
        } else {
            command_imu_ok(PSTR("stop, not running"));
        }
    }
    else if(count == 2 && strcmp(args[1], "read") == 0){
        if(imu_read_once(now)){
            command_imu_ok(PSTR("read, init then one sample"));
        } else {
            command_imu_warning(PSTR("imu read only when stopped, use imu stop first"));
        }
    }
    else if(count == 2 && strcmp(args[1], "status") == 0){
        imu_request_report(now);
    }
    else {
        command_imu_warning(PSTR("use imu start / stop / read / status"));
    }
}
