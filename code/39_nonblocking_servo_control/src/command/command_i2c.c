#include "command/command_i2c.h"
#include "scanner.h"
#include "usart.h"

#include <string.h>
#include <avr/pgmspace.h>

static void command_i2c_ok(const char *text)
{
    usart0_send_string_P(PSTR("OK i2c "));
    usart0_send_string_P(text);
    usart0_send_newline();
}

void command_i2c_execute(char *args[], uint8_t count, uint32_t now)
{
    if(count == 2 && strcmp(args[1], "scan") == 0){
        if(scanner_start(now)){
            command_i2c_ok(PSTR("scan 0x08-0x77"));
        } else {
            command_i2c_ok(PSTR("scan already running"));
        }
    }
    else if(count == 2 && strcmp(args[1], "stop") == 0){
        if(scanner_stop(now)){
            command_i2c_ok(PSTR("stop"));
        } else {
            command_i2c_ok(PSTR("stop, no scan running"));
        }
    }
    else if(count == 2 && strcmp(args[1], "status") == 0){
        scanner_request_report(now);
    }
    else {
        usart0_send_string_P(PSTR("WARNING use i2c scan / stop / status"));
        usart0_send_newline();
    }
}
