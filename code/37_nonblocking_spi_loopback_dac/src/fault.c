#include "fault.h"
#include "timer.h"
#include "usart.h"
#include <avr/pgmspace.h>

static fault_t current_fault = FAULT_NONE;

void fault_set(fault_t fault, uint32_t now)
{
    if(fault == FAULT_NONE || fault == current_fault){
        return;
    }

    if(current_fault != FAULT_NONE && fault != FAULT_ESTOP){
        return;
    }

    current_fault = fault;
    timer1_lock();

    usart0_send_string_P(PSTR("["));
    usart0_send_uint32(now);
    usart0_send_string_P(PSTR(" ms] FAULT: "));
    fault_send_name(fault);
    usart0_send_string_P(PSTR(", type reset"));
    usart0_send_newline();
}

void fault_clear(void)
{
    current_fault = FAULT_NONE;
}

bool fault_active(void)
{
    return current_fault != FAULT_NONE;
}

fault_t fault_get(void)
{
    return current_fault;
}

void fault_send_name(fault_t fault)
{
    switch(fault){
        case FAULT_NONE:
            usart0_send_string_P(PSTR("NONE"));
            break;
        case FAULT_ESTOP:
            usart0_send_string_P(PSTR("ESTOP"));
            break;
        case FAULT_LIMIT_WIRING:
            usart0_send_string_P(PSTR("LIMIT_WIRING"));
            break;
        case FAULT_HOME_NOT_FOUND:
            usart0_send_string_P(PSTR("HOME_NOT_FOUND"));
            break;
        case FAULT_SWITCH_STUCK:
            usart0_send_string_P(PSTR("SWITCH_STUCK"));
            break;
        case FAULT_HOME_WRONG_LIMIT:
            usart0_send_string_P(PSTR("HOME_WRONG_LIMIT"));
            break;
        case FAULT_MOTION_STOPPED:
            usart0_send_string_P(PSTR("MOTION_STOPPED"));
            break;
        case FAULT_CONFIG:
            usart0_send_string_P(PSTR("CONFIG"));
            break;
        case FAULT_LIMIT_UNEXPECTED:
            usart0_send_string_P(PSTR("LIMIT_UNEXPECTED"));
            break;
        case FAULT_SOFT_LIMIT:
            usart0_send_string_P(PSTR("SOFT_LIMIT"));
            break;
    }
}
