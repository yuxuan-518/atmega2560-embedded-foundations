#include "command/command_range.h"
#include "command.h"
#include "range.h"
#include "report.h"
#include "usart.h"

#include <string.h>
#include <avr/pgmspace.h>

static void command_range_ok(const char *text)
{
    usart0_send_string_P(PSTR("OK range "));
    usart0_send_string_P(text);
    usart0_send_newline();
}

static void command_range_warning(const char *text)
{
    usart0_send_string_P(PSTR("WARNING "));
    usart0_send_string_P(text);
    usart0_send_newline();
}

void command_range_execute(char *args[], uint8_t count, uint32_t now)
{
    int32_t value = 0;

    if(count == 2 && strcmp(args[1], "start") == 0){
        if(range_is_running()){
            command_range_ok(PSTR("already running"));
            return;
        }
        range_start(now);
        command_range_ok(PSTR("start"));
    }
    else if(count == 2 && strcmp(args[1], "stop") == 0){
        range_stop();
        command_range_ok(PSTR("stop, TRIG low"));
    }
    else if(count == 2 && strcmp(args[1], "status") == 0){
        report_request_range();
    }
    else if(count == 3 && strcmp(args[1], "filter") == 0 && strcmp(args[2], "off") == 0){
        range_set_filter(RANGE_FILTER_OFF, now);
        command_range_ok(PSTR("filter off"));
    }
    else if(count == 3 && strcmp(args[1], "filter") == 0 && strcmp(args[2], "mean") == 0){
        range_set_filter(RANGE_FILTER_MEAN, now);
        command_range_ok(PSTR("filter mean 5"));
    }
    else if(count == 3 && strcmp(args[1], "filter") == 0 && strcmp(args[2], "median") == 0){
        range_set_filter(RANGE_FILTER_MEDIAN, now);
        command_range_ok(PSTR("filter median 5"));
    }
    else if(count == 3 && strcmp(args[1], "alarm") == 0){
        if(!command_parse_int32(args[2], &value) || value < 0 || !range_set_alarm(value, now)){
            command_range_warning(PSTR("range alarm must be 0 (off) or 20-4000 mm"));
            return;
        }
        command_range_ok(PSTR("alarm"));
    }
    else {
        command_range_warning(PSTR("use range start / stop / filter off|mean|median / alarm <mm> / status"));
    }
}
