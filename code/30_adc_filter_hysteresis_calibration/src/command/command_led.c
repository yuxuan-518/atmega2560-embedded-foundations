#include "command/command_led.h"
#include "command.h"
#include "config.h"
#include "led.h"
#include "pot.h"
#include "report.h"
#include "usart.h"

#include <string.h>

static void command_led_ok(const char text[])
{
    usart0_send_string("OK led ");
    usart0_send_string(text);
    usart0_send_newline();
}

static void command_led_warning(const char text[])
{
    usart0_send_string("WARNING ");
    usart0_send_string(text);
    usart0_send_newline();
}

void command_led_execute(char *args[], uint8_t count, uint32_t now)
{
    int32_t value = 0;

    if(count == 3 && strcmp(args[1], "duty") == 0){
        if(!command_parse_int32(args[2], &value) || value < 0 || value > 255){
            command_led_warning("led duty must be 0-255");
            return;
        }

        led_set_duty(value);
        command_led_ok("duty");
    }
    else if(count == 2 && strcmp(args[1], "breathe") == 0){
        led_breathe(now);
        command_led_ok("breathe");
    }
    else if(count == 3 && strcmp(args[1], "period") == 0){
        if(!command_parse_int32(args[2], &value) || value <= 0 || !led_set_period(value)){
            command_led_warning("led period must be 200-60000 ms");
            return;
        }

        command_led_ok("period");
    }
    else if((count == 2 || count == 3) && strcmp(args[1], "pot") == 0){
        int32_t new_source = CONFIG_POT_DEFAULT_SOURCE;

        if(count == 3){
            if(!command_parse_int32(args[2], &new_source) || new_source < 0 || new_source >= CONFIG_ADC_COUNT){
                command_led_warning("led pot source must be 0 (A0) or 1 (A1)");
                return;
            }
        }

        pot_set_source(new_source);
        led_pot(pot_source_valid(), pot_source_duty());

        if(pot_source_valid()){
            command_led_ok("pot");
        } else {
            command_led_ok("pot, waiting for first valid sample");
        }
    }
    else if(count == 2 && strcmp(args[1], "off") == 0){
        led_off();
        command_led_ok("off");
    }
    else if(count == 2 && strcmp(args[1], "status") == 0){
        report_request_led();
    }
    else {
        command_led_warning("use led duty <0-255> / breathe / period <ms> / pot [0|1] / off / status");
    }
}
