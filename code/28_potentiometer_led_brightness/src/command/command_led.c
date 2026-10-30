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
    else if(count == 2 && strcmp(args[1], "pot") == 0){
        led_pot(pot_is_valid(), pot_raw_to_duty(pot_get_raw()));

        if(pot_is_valid()){
            command_led_ok("pot");
        } else {
            command_led_ok("pot, waiting for first ADC sample");
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
        command_led_warning("use led duty <0-255> / breathe / period <ms> / pot / off / status");
    }
}
