#include "command/command_adc.h"
#include "report.h"
#include "usart.h"

#include <string.h>
#include <avr/pgmspace.h>

void command_adc_execute(char *args[], uint8_t count)
{
    if(count == 2 && strcmp(args[1], "status") == 0){
        report_request_adc();
        return;
    }

    usart0_send_string_P(PSTR("WARNING use adc status"));
    usart0_send_newline();
}
