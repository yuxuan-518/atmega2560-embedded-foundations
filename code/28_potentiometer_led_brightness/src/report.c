#include "report.h"
#include "config.h"
#include "motor.h"
#include "usart.h"
#include "nh3.h"
#include "led.h"
#include "pot.h"
#include "pwm.h"
#include "command/command_motor.h"

#include <string.h>

static const char *const help_lines[] = {
    "move <steps>     +MAX / -MIN, |steps| <= 1000000",
    "goto <pos>       homed only, inside soft limit",
    "repeat <a> <b> <n>  n = 1-1000 cycles",
    "start / run min / run max / reverse / stop",
    "home / exit / reset / status / config",
    "speed <Hz> / accel <Hz/s>|low|high",
    "set start|max|softmin|softmax <v>",
    "demo / log on / log off",
    "nh3 / nh3 raw on|off / nh3 baud <v> / nh3 active / nh3 clear",
    "led duty <0-255> / led breathe / led period <ms> / led off / led status",
    "led pot / adc status"
};

#define REPORT_HELP_COUNT (sizeof(help_lines) / sizeof(help_lines[0]))

static uint8_t help_index = REPORT_HELP_COUNT;
static bool status_pending = false;
static bool config_pending = false;
static bool nh3_pending = false;
static bool led_pending = false;
static bool adc_pending = false;
static nh3_state_t last_nh3_state = NH3_NO_DATA;
static bool log_enabled = false;
static uint32_t last_heartbeat = 0;
static uint32_t last_log = 0;
static uint32_t dropped_total = 0;
static motor_state_t last_state = MOTOR_STOPPED;

static bool report_room(uint16_t bytes)
{
    return usart0_tx_free() >= bytes + REPORT_TX_RESERVE;
}

static void report_send_int32(int32_t number)
{
    if(number < 0){
        usart0_send_string("-");
        number = -number;
    }
    usart0_send_uint32(number);
}

void report_update(uint32_t now)
{
    motor_state_t state = motor_get_state();

    if(state != last_state && report_room(REPORT_LINE_BYTES)){
        last_state = state;
        usart0_send_string("[");
        usart0_send_uint32(now);
        usart0_send_string(" ms] ");
        command_motor_send_state(state);
        usart0_send_string(" ");
        usart0_send_uint32(motor_get_current_hz());
        usart0_send_string(" Hz");
        usart0_send_newline();
    }

    nh3_state_t nh3_state = nh3_get_state();

    if(nh3_state != last_nh3_state && report_room(REPORT_LINE_BYTES)){
        last_nh3_state = nh3_state;
        usart0_send_string("[");
        usart0_send_uint32(now);
        usart0_send_string(" ms] NH3 ");
        nh3_send_state(nh3_state);
        usart0_send_newline();
    }

    while(help_index < REPORT_HELP_COUNT && report_room(strlen(help_lines[help_index]) + 2)){
        usart0_send_string(help_lines[help_index]);
        usart0_send_newline();
        help_index++;
    }

    if(status_pending && report_room(REPORT_STATUS_BYTES)){
        status_pending = false;
        command_motor_send_status();
    }

    if(config_pending && report_room(REPORT_CONFIG_BYTES)){
        config_pending = false;
        config_send();
    }

    if(nh3_pending && report_room(REPORT_NH3_BYTES)){
        nh3_pending = false;
        nh3_send_status(now);
    }

    if(led_pending && report_room(REPORT_LED_BYTES)){
        led_pending = false;
        led_send_status();
    }

    if(adc_pending && report_room(REPORT_ADC_BYTES)){
        adc_pending = false;
        pot_send_status(now);
    }

    if(log_enabled && now - last_log >= REPORT_LOG_MS){
        last_log = now;

        if(report_room(REPORT_LOG_BYTES)){
            usart0_send_string("LOG,");
            usart0_send_uint32(now);
            usart0_send_string(",");
            usart0_send_uint32(motor_get_current_hz());
            usart0_send_string(",");
            report_send_int32(motor_get_position());
            usart0_send_string(",");

            uint32_t ppb = 0;
            if(nh3_get_ppb(&ppb)){
                usart0_send_uint32(ppb);
            } else {
                usart0_send_string("-");
            }
            usart0_send_string(",");

            if(pot_is_valid()){
                usart0_send_uint32(pot_get_raw());
            } else {
                usart0_send_string("-");
            }
            usart0_send_string(",");
            usart0_send_uint32(pwm4a_get_duty());
            usart0_send_newline();
        }
    }

    if(now - last_heartbeat >= REPORT_HEARTBEAT_MS && report_room(REPORT_STATUS_BYTES + REPORT_NH3_BRIEF_BYTES)){
        last_heartbeat = now;

        dropped_total += usart0_tx_take_dropped();

        usart0_send_string("TIME: ");
        usart0_send_uint32(now / 1000);
        usart0_send_string(" s, ");
        command_motor_send_status();
        nh3_send_brief(now);

        if(dropped_total != 0){
            usart0_send_string("TX DROPPED: ");
            usart0_send_uint32(dropped_total);
            usart0_send_newline();
        }
    }
}

void report_request_status(void)
{
    status_pending = true;
}

void report_request_config(void)
{
    config_pending = true;
}

void report_request_help(void)
{
    help_index = 0;
}

void report_request_nh3(void)
{
    nh3_pending = true;
}

void report_request_led(void)
{
    led_pending = true;
}

void report_request_adc(void)
{
    adc_pending = true;
}

void report_set_log(bool enable)
{
    log_enabled = enable;
}
