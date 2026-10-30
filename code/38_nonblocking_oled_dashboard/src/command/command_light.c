#include "command/command_light.h"
#include "command.h"
#include "light.h"
#include "report.h"
#include "usart.h"

#include <string.h>
#include <avr/pgmspace.h>

static void command_light_ok(const char *text)
{
    usart0_send_string_P(PSTR("OK light "));
    usart0_send_string_P(text);
    usart0_send_newline();
}

static void command_light_warning(const char *text)
{
    usart0_send_string_P(PSTR("WARNING "));
    usart0_send_string_P(text);
    usart0_send_newline();
}

void command_light_execute(char *args[], uint8_t count)
{
    int32_t a = 0;
    int32_t b = 0;

    if(count == 2 && strcmp(args[1], "status") == 0){
        report_request_light();
    }
    else if(count == 4 && strcmp(args[1], "thresholds") == 0){
        if(!command_parse_int32(args[2], &a) || !command_parse_int32(args[3], &b) ||
           a < 0 || b > 1023 || a >= b || !light_set_thresholds(a, b)){
            command_light_warning(PSTR("light thresholds need 0 <= on < off <= 1023"));
            return;
        }
        command_light_ok(PSTR("thresholds"));
    }
    else if(count == 3 && strcmp(args[1], "window") == 0){
        if(!command_parse_int32(args[2], &a) || a < 1 || a > 32 || !light_set_window(a)){
            command_light_warning(PSTR("light window must be 1-32"));
            return;
        }
        command_light_ok(PSTR("window"));
    }
    else if(count == 3 && strcmp(args[1], "polarity") == 0 && strcmp(args[2], "normal") == 0){
        light_set_invert(false);
        command_light_ok(PSTR("polarity normal"));
    }
    else if(count == 3 && strcmp(args[1], "polarity") == 0 && strcmp(args[2], "invert") == 0){
        light_set_invert(true);
        command_light_ok(PSTR("polarity invert"));
    }
    else {
        command_light_warning(PSTR("use light status / thresholds <on> <off> / window <1-32> / polarity normal|invert"));
    }
}
