#include "command/command_counter.h"
#include "counter.h"
#include "report.h"
#include "usart.h"

#include <string.h>
#include <avr/pgmspace.h>

static void command_counter_ok(const char *text)
{
    usart0_send_string_P(PSTR("OK counter "));
    usart0_send_string_P(text);
    usart0_send_newline();
}

static void command_counter_warning(const char *text)
{
    usart0_send_string_P(PSTR("WARNING "));
    usart0_send_string_P(text);
    usart0_send_newline();
}

void command_counter_execute(char *args[], uint8_t count, uint32_t now)
{
    if(count == 2 && strcmp(args[1], "start") == 0){
        if(counter_is_running()){
            command_counter_ok(PSTR("already running"));
            return;
        }
        counter_start(now);
        command_counter_ok(PSTR("start"));
    }
    else if(count == 2 && strcmp(args[1], "stop") == 0){
        counter_stop();
        command_counter_ok(PSTR("stop"));
    }
    else if(count == 2 && strcmp(args[1], "clear") == 0){
        counter_clear(now);
        command_counter_ok(PSTR("clear"));
    }
    else if(count == 2 && strcmp(args[1], "status") == 0){
        report_request_counter();
    }
    else if(count == 3 && strcmp(args[1], "edge") == 0 && strcmp(args[2], "rising") == 0){
        counter_set_rising(true, now);
        command_counter_ok(PSTR("edge rising, counts cleared"));
    }
    else if(count == 3 && strcmp(args[1], "edge") == 0 && strcmp(args[2], "falling") == 0){
        counter_set_rising(false, now);
        command_counter_ok(PSTR("edge falling, counts cleared"));
    }
    else if(count == 3 && strcmp(args[1], "mode") == 0 && strcmp(args[2], "pulse") == 0){
        counter_set_mode(COUNTER_PULSE, now);
        command_counter_ok(PSTR("mode pulse, counts cleared"));
    }
    else if(count == 3 && strcmp(args[1], "mode") == 0 && strcmp(args[2], "button") == 0){
        counter_set_mode(COUNTER_BUTTON, now);
        command_counter_ok(PSTR("mode button, counts cleared"));
    }
    else {
        command_counter_warning(PSTR("use counter start / stop / clear / edge rising|falling / mode pulse|button / status"));
    }
}
