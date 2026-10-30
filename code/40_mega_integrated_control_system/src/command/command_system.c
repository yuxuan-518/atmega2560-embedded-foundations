#include "command/command_system.h"
#include "command.h"
#include "config.h"
#include "system.h"
#include "autorun.h"
#include "task.h"
#include "fault.h"
#include "homing.h"
#include "usart.h"

#include <string.h>
#include <avr/pgmspace.h>

static void command_system_warning(const char *text)
{
    usart0_send_string_P(PSTR("WARNING "));
    usart0_send_string_P(text);
    usart0_send_newline();
}

static void command_system_send_int32(int32_t number)
{
    if(number < 0){
        usart0_send_string_P(PSTR("-"));
        number = -number;
    }
    usart0_send_uint32((uint32_t)number);
}

static bool command_system_need_idle(void)
{
    sys_state_t state = system_get_state();

    if(state == SYS_IDLE){
        return true;
    }

    usart0_send_string_P(PSTR("WARNING plan can only change in IDLE, now "));
    system_send_state(state);
    usart0_send_string_P(PSTR(", nothing changed"));
    usart0_send_newline();
    return false;
}

static void command_system_send_soft(void)
{
    const config_t *cfg = config_get();

    usart0_send_string_P(PSTR("WARNING points must be inside soft limit "));
    command_system_send_int32(cfg->soft_min);
    usart0_send_string_P(PSTR(".."));
    command_system_send_int32(cfg->soft_max);
    usart0_send_newline();
}

static void command_system_home(void)
{
    if(fault_active()){
        usart0_send_string_P(PSTR("WARNING FAULT: "));
        fault_send_name(fault_get());
        usart0_send_string_P(PSTR(", type reset"));
        usart0_send_newline();
        return;
    }

    if(task_busy()){
        usart0_send_string_P(PSTR("WARNING busy: "));
        task_send_name();
        usart0_send_string_P(PSTR(", type system stop first"));
        usart0_send_newline();
        return;
    }

    homing_start();
    usart0_send_string_P(PSTR("OK system home, homing started"));
    usart0_send_newline();
}

static void command_system_points(char *args[])
{
    int32_t a = 0;
    int32_t b = 0;

    if(!command_parse_int32(args[2], &a) || !command_parse_int32(args[3], &b)){
        command_system_warning(PSTR("use system points <A> <B>"));
        return;
    }

    if(!command_system_need_idle()){
        return;
    }

    auto_set_t result = autorun_set_points(a, b);

    if(result == AUTO_SET_SAME){
        command_system_warning(PSTR("points A and B must differ"));
    } else if(result == AUTO_SET_RANGE){
        command_system_send_soft();
    } else if(result != AUTO_SET_OK){
        command_system_warning(PSTR("plan busy, nothing changed"));
    } else {
        usart0_send_string_P(PSTR("OK system points A "));
        command_system_send_int32(a);
        usart0_send_string_P(PSTR(" B "));
        command_system_send_int32(b);
        usart0_send_newline();
    }
}

static void command_system_cycles(char *args[])
{
    int32_t value = 0;

    if(!command_parse_int32(args[2], &value)){
        command_system_warning(PSTR("use system cycles <1-1000>"));
        return;
    }

    if(!command_system_need_idle()){
        return;
    }

    if(autorun_set_cycles(value) != AUTO_SET_OK){
        command_system_warning(PSTR("cycles must be 1-1000"));
        return;
    }

    usart0_send_string_P(PSTR("OK system cycles "));
    usart0_send_uint32((uint32_t)value);
    usart0_send_string_P(PSTR(" (1 cycle = A->B->A)"));
    usart0_send_newline();
}

static void command_system_progress(void)
{
    usart0_send_string_P(PSTR(", LEG "));
    usart0_send_uint32(autorun_get_leg() + 1UL);
    usart0_send_string_P(PSTR("/"));
    usart0_send_uint32(autorun_get_legs_total());
    usart0_send_string_P(PSTR(", CYCLES DONE "));
    usart0_send_uint32(autorun_get_cycles_done());
    usart0_send_string_P(PSTR("/"));
    usart0_send_uint32(autorun_get_cycles());
    usart0_send_newline();
}

static void command_system_start(uint32_t now)
{
    auto_phase_t phase = autorun_get_phase();

    if(phase == AUTO_RUNNING){
        usart0_send_string_P(PSTR("OK system already running, progress kept"));
        command_system_progress();
        return;
    }

    if(phase == AUTO_ENDING){
        command_system_warning(PSTR("system stopping, start again when IDLE"));
        return;
    }

    auto_start_t result = autorun_start(now);

    if(result == AUTO_START_FAULT){
        usart0_send_string_P(PSTR("WARNING FAULT: "));
        fault_send_name(fault_get());
        usart0_send_string_P(PSTR(", type reset, then system home"));
        usart0_send_newline();
    } else if(result == AUTO_START_BUSY){
        usart0_send_string_P(PSTR("WARNING busy: "));
        task_send_name();
        usart0_send_string_P(PSTR(", type system stop first"));
        usart0_send_newline();
    } else if(result == AUTO_START_NOT_HOMED){
        command_system_warning(PSTR("not homed, type system home first"));
    } else if(result == AUTO_START_NO_POINTS){
        command_system_warning(PSTR("points not set, use system points <A> <B>"));
    } else if(result == AUTO_START_RANGE){
        command_system_send_soft();
    } else if(result == AUTO_START_LIMIT){
        command_system_warning(PSTR("limit active, use exit"));
    } else if(result == AUTO_START_MOTION){
        command_system_warning(PSTR("motion refused, nothing started"));
    } else {
        usart0_send_string_P(PSTR("OK system start A "));
        command_system_send_int32(autorun_get_a());
        usart0_send_string_P(PSTR(" B "));
        command_system_send_int32(autorun_get_b());
        usart0_send_string_P(PSTR(", "));
        usart0_send_uint32(autorun_get_cycles());
        usart0_send_string_P(PSTR(" cycles, first to A"));
        usart0_send_newline();
    }
}

static void command_system_stop(void)
{
    if(!task_busy()){
        usart0_send_string_P(PSTR("OK system stop, nothing running"));
        usart0_send_newline();
        return;
    }

    task_stop();
    usart0_send_string_P(PSTR("OK system stop, remaining actions cancelled, no auto resume"));
    usart0_send_newline();
}

void command_system_execute(char *args[], uint8_t count, uint32_t now)
{
    if(count == 2 && strcmp(args[1], "home") == 0){
        command_system_home();
    }
    else if(count == 4 && strcmp(args[1], "points") == 0){
        command_system_points(args);
    }
    else if(count == 3 && strcmp(args[1], "cycles") == 0){
        command_system_cycles(args);
    }
    else if(count == 2 && strcmp(args[1], "start") == 0){
        command_system_start(now);
    }
    else if(count == 2 && strcmp(args[1], "stop") == 0){
        command_system_stop();
    }
    else if(count == 2 && strcmp(args[1], "status") == 0){
        system_request_report(now);
    }
    else {
        command_system_warning(PSTR("use system home / points <A> <B> / cycles <n> / start / stop / status"));
    }
}
