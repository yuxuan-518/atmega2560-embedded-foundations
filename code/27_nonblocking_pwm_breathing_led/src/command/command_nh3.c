#include "command/command_nh3.h"
#include "command.h"
#include "nh3.h"
#include "report.h"
#include "task.h"
#include "usart.h"

#include <string.h>

static void command_nh3_ok(const char text[])
{
    usart0_send_string("OK nh3 ");
    usart0_send_string(text);
    usart0_send_newline();
}

static void command_nh3_warning(const char text[])
{
    usart0_send_string("WARNING ");
    usart0_send_string(text);
    usart0_send_newline();
}

void command_nh3_execute(char *args[], uint8_t count)
{
    if(count == 1){
        report_request_nh3();
    }
    else if(count == 3 && strcmp(args[1], "raw") == 0 && strcmp(args[2], "on") == 0){
        nh3_set_raw(true);
        command_nh3_ok("raw on");
    }
    else if(count == 3 && strcmp(args[1], "raw") == 0 && strcmp(args[2], "off") == 0){
        nh3_set_raw(false);
        command_nh3_ok("raw off");
    }
    else if(count == 3 && strcmp(args[1], "baud") == 0){
        int32_t value = 0;

        if(!command_parse_int32(args[2], &value) || value <= 0 || !nh3_set_baud(value)){
            command_nh3_warning("nh3 baud must be 1200-115200");
            return;
        }
        command_nh3_ok("baud");
    }
    else if(count == 2 && strcmp(args[1], "active") == 0){
        if(nh3_send_active_command()){
            command_nh3_ok("active upload command sent");
        } else {
            command_nh3_warning("protocol has no mode command");
        }
    }
    else if(count == 2 && strcmp(args[1], "clear") == 0){
        nh3_reset_counters();
        command_nh3_ok("counters cleared");
    }
    else {
        command_nh3_warning("use nh3 / nh3 raw on|off / nh3 baud <v> / nh3 active / nh3 clear");
    }
}
