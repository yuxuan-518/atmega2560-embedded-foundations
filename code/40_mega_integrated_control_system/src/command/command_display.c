#include "command/command_display.h"
#include "command.h"
#include "config.h"
#include "display.h"
#include "display_view.h"
#include "usart.h"

#include <string.h>
#include <avr/pgmspace.h>

static void command_display_ok(const char *text)
{
    usart0_send_string_P(PSTR("OK display "));
    usart0_send_string_P(text);
    usart0_send_newline();
}

static void command_display_warning(const char *text)
{
    usart0_send_string_P(PSTR("WARNING "));
    usart0_send_string_P(text);
    usart0_send_newline();
}

void command_display_execute(char *args[], uint8_t count, uint32_t now)
{
    int32_t value = 0;

    if(count == 2 && strcmp(args[1], "on") == 0){
        display_on_t result = display_on(now);

        if(result == DISPLAY_ON_OK){
            command_display_ok(PSTR("on, init then full refresh"));
        } else if(result == DISPLAY_ON_ALREADY){
            command_display_ok(PSTR("already on"));
        } else if(result == DISPLAY_ON_STOPPING){
            command_display_warning(PSTR("display still turning off, try again"));
        } else {
            command_display_warning(PSTR("display address must be 0x3C or 0x3D"));
        }
    }
    else if(count == 2 && strcmp(args[1], "off") == 0){
        if(display_off()){
            command_display_ok(PSTR("off"));
        } else {
            command_display_ok(PSTR("already off"));
        }
    }
    else if(count == 3 && strcmp(args[1], "page") == 0){
        if(!command_parse_int32(args[2], &value) || value < 0 || value >= CONFIG_DISPLAY_PAGES || !display_set_page((uint8_t)value)){
            command_display_warning(PSTR("use display page <0-3>"));
            return;
        }
        usart0_send_string_P(PSTR("OK display page "));
        usart0_send_uint32((uint32_t)value);
        usart0_send_string_P(PSTR(" "));
        display_view_send_page_name((uint8_t)value);
        usart0_send_newline();
    }
    else if(count == 2 && strcmp(args[1], "refresh") == 0){
        if(display_refresh()){
            command_display_ok(PSTR("refresh, full frame queued"));
        } else {
            command_display_warning(PSTR("display is off, use display on"));
        }
    }
    else if(count == 2 && strcmp(args[1], "status") == 0){
        display_request_report(now);
    }
    else {
        command_display_warning(PSTR("use display on / off / page <0-3> / refresh / status"));
    }
}
