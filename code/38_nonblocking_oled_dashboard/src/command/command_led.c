#include "command/command_led.h"
#include "command.h"
#include "config.h"
#include "led.h"
#include "pot.h"
#include "light.h"
#include "report.h"
#include "usart.h"

#include <string.h>
#include <avr/pgmspace.h>

static void command_led_ok(const char *text)
{
    usart0_send_string_P(PSTR("OK led "));
    usart0_send_string_P(text);
    usart0_send_newline();
}

static void command_led_warning(const char *text)
{
    usart0_send_string_P(PSTR("WARNING "));
    usart0_send_string_P(text);
    usart0_send_newline();
}

void command_led_execute(char *args[], uint8_t count, uint32_t now)
{
    int32_t value = 0;

    if(count == 3 && strcmp(args[1], "duty") == 0){
        if(!command_parse_int32(args[2], &value) || value < 0 || value > 255){
            command_led_warning(PSTR("led duty must be 0-255"));
            return;
        }

        led_set_duty(value);
        command_led_ok(PSTR("duty"));
    }
    else if(count == 2 && strcmp(args[1], "breathe") == 0){
        led_breathe(now);
        command_led_ok(PSTR("breathe"));
    }
    else if(count == 3 && strcmp(args[1], "period") == 0){
        if(!command_parse_int32(args[2], &value) || value <= 0 || !led_set_period(value)){
            command_led_warning(PSTR("led period must be 200-60000 ms"));
            return;
        }

        command_led_ok(PSTR("period"));
    }
    else if((count == 2 || count == 3) && strcmp(args[1], "pot") == 0){
        int32_t new_source = CONFIG_POT_DEFAULT_SOURCE;

        if(count == 3){
            if(!command_parse_int32(args[2], &new_source) || new_source < 0 || new_source >= CONFIG_POT_COUNT){
                command_led_warning(PSTR("led pot source must be 0 (A0) or 1 (A1)"));
                return;
            }
        }

        pot_set_source(new_source);
        led_pot(pot_source_valid(), pot_source_duty());

        if(pot_source_valid()){
            command_led_ok(PSTR("pot"));
        } else {
            command_led_ok(PSTR("pot, waiting for first valid sample"));
        }
    }
    else if(count == 2 && strcmp(args[1], "auto") == 0){
        led_auto(light_lamp_on());

        if(light_get_data() == LIGHT_OK){
            command_led_ok(PSTR("auto"));
        } else {
            command_led_ok(PSTR("auto, LED off until fresh light data"));
        }
    }
    else if(count == 2 && strcmp(args[1], "off") == 0){
        led_off();
        command_led_ok(PSTR("off"));
    }
    else if(count == 2 && strcmp(args[1], "status") == 0){
        report_request_led();
    }
    else {
        command_led_warning(PSTR("use led duty <0-255> / breathe / period <ms> / pot [0|1] / auto / off / status"));
    }
}
