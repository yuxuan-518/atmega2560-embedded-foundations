#include "command/command_pot.h"
#include "command.h"
#include "config.h"
#include "pot.h"
#include "report.h"
#include "usart.h"

#include <string.h>
#include <avr/pgmspace.h>

static uint8_t config_channel = 0;

static void command_pot_ok(const char *text)
{
    usart0_send_string_P(PSTR("OK pot "));
    usart0_send_string_P(text);
    usart0_send_string_P(PSTR(" (ch "));
    usart0_send_uint32(config_channel);
    usart0_send_string_P(PSTR(")"));
    usart0_send_newline();
}

static void command_pot_warning(const char *text)
{
    usart0_send_string_P(PSTR("WARNING "));
    usart0_send_string_P(text);
    usart0_send_newline();
}

uint8_t command_pot_config_channel(void)
{
    return config_channel;
}

void command_pot_execute(char *args[], uint8_t count)
{
    int32_t a = 0;
    int32_t b = 0;

    if(count == 2 && strcmp(args[1], "status") == 0){
        report_request_pot();
    }
    else if(count == 3 && strcmp(args[1], "ch") == 0){
        if(!command_parse_int32(args[2], &a) || a < 0 || a >= CONFIG_POT_COUNT){
            command_pot_warning(PSTR("pot ch must be 0 or 1"));
            return;
        }
        config_channel = a;
        command_pot_ok(PSTR("ch"));
    }
    else if(count == 3 && strcmp(args[1], "window") == 0){
        if(!command_parse_int32(args[2], &a) || a < 1 || a > 32 || !pot_set_window(config_channel, a)){
            command_pot_warning(PSTR("pot window must be 1-32"));
            return;
        }
        command_pot_ok(PSTR("window"));
    }
    else if(count == 3 && strcmp(args[1], "hyst") == 0){
        if(!command_parse_int32(args[2], &a) || a < 0 || a > CONFIG_POT_HYST_MAX || !pot_set_hyst(config_channel, a)){
            command_pot_warning(PSTR("pot hyst must be 0-64"));
            return;
        }
        command_pot_ok(PSTR("hyst"));
    }
    else if(count == 4 && strcmp(args[1], "cal") == 0){
        if(!command_parse_int32(args[2], &a) || !command_parse_int32(args[3], &b) ||
           a < 0 || b > 1023 || a >= b || !pot_set_cal(config_channel, a, b)){
            command_pot_warning(PSTR("pot cal needs 0 <= low < high <= 1023"));
            return;
        }
        command_pot_ok(PSTR("cal"));
    }
    else if(count == 2 && strcmp(args[1], "default") == 0){
        pot_set_default(config_channel);
        command_pot_ok(PSTR("default"));
    }
    else {
        command_pot_warning(PSTR("use pot status / ch <0|1> / window <1-32> / hyst <0-64> / cal <low> <high> / default"));
    }
}
