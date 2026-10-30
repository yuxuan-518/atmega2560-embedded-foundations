#include "command/command_dac.h"
#include "command.h"
#include "dac.h"
#include "mcp4921.h"
#include "usart.h"

#include <string.h>
#include <avr/pgmspace.h>

static void command_dac_warning(const char *text)
{
    usart0_send_string_P(PSTR("WARNING "));
    usart0_send_string_P(text);
    usart0_send_newline();
}

void command_dac_execute(char *args[], uint8_t count, uint32_t now)
{
    int32_t value = 0;

    if(count == 3 && strcmp(args[1], "set") == 0){
        if(!command_parse_int32(args[2], &value) || value < 0 || value > MCP4921_MAX){
            command_dac_warning(PSTR("use dac set <0-4095>"));
            return;
        }

        if(!dac_is_ready()){
            command_dac_warning(PSTR("dac disabled, DAC SPI config invalid"));
            return;
        }

        bool replaced = dac_has_pending();
        bool after = dac_get_state() != DAC_IDLE;

        dac_set((uint16_t)value, now);
        usart0_send_string_P(PSTR("OK dac set "));
        usart0_send_uint32((uint32_t)value);

        if(replaced){
            usart0_send_string_P(PSTR(" queued, replaces older request"));
        } else if(after){
            usart0_send_string_P(PSTR(" queued after current write"));
        } else {
            usart0_send_string_P(PSTR(" queued"));
        }
        usart0_send_newline();
    }
    else if(count == 2 && strcmp(args[1], "status") == 0){
        dac_request_report();
    }
    else {
        command_dac_warning(PSTR("use dac set <0-4095> / dac status"));
    }
}
