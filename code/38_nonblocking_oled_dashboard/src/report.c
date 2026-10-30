#include "report.h"
#include "config.h"
#include "motor.h"
#include "usart.h"
#include "nh3.h"
#include "led.h"
#include "pot.h"
#include "analog.h"
#include "light.h"
#include "buzzer.h"
#include "counter.h"
#include "range.h"
#include "scanner.h"
#include "imu.h"
#include "loopback.h"
#include "dac.h"
#include "display.h"
#include <avr/pgmspace.h>
#include "pwm.h"
#include "command/command_pot.h"
#include "command/command_motor.h"

#include <string.h>

static const char help_0[] PROGMEM = "move <steps>     +MAX / -MIN, |steps| <= 1000000";
static const char help_1[] PROGMEM = "goto <pos>       homed only, inside soft limit";
static const char help_2[] PROGMEM = "repeat <a> <b> <n>  n = 1-1000 cycles";
static const char help_3[] PROGMEM = "start / run min / run max / reverse / stop";
static const char help_4[] PROGMEM = "home / exit / reset / status / config";
static const char help_5[] PROGMEM = "speed <Hz> / accel <Hz/s>|low|high";
static const char help_6[] PROGMEM = "set start|max|softmin|softmax <v>";
static const char help_7[] PROGMEM = "demo / log on / log off";
static const char help_8[] PROGMEM = "nh3 / nh3 raw on|off / nh3 baud <v> / nh3 active / nh3 clear";
static const char help_9[] PROGMEM = "led duty <0-255> / led breathe / led period <ms> / led off / led status";
static const char help_10[] PROGMEM = "led pot [0|1] / adc status";
static const char help_11[] PROGMEM = "pot status / pot ch <0|1> / pot window <1-32>";
static const char help_12[] PROGMEM = "pot hyst <0-64> / pot cal <low> <high> / pot default";
static const char help_13[] PROGMEM = "led auto / light status / light thresholds <on> <off>";
static const char help_14[] PROGMEM = "light window <1-32> / light polarity normal|invert";
static const char help_15[] PROGMEM = "buzzer tone <Hz 0|100-5000> <ms 1-10000> / buzzer stop";
static const char help_16[] PROGMEM = "buzzer play scale|melody / buzzer status";
static const char help_17[] PROGMEM = "counter start / stop / clear / status (D3 INT5)";
static const char help_18[] PROGMEM = "counter edge rising|falling / counter mode pulse|button";
static const char help_19[] PROGMEM = "range start / stop / status (TRIG D7, ECHO D10)";
static const char help_20[] PROGMEM = "range filter off|mean|median / range alarm <0|20-4000 mm>";
static const char help_21[] PROGMEM = "i2c scan / i2c stop / i2c status (SDA D20, SCL D21)";
static const char help_22[] PROGMEM = "imu start / imu stop / imu read / imu status (MPU-6050)";
static const char help_23[] PROGMEM = "spi loopback / spi mode <0-3> / spi clock <Hz> / spi stop / spi status";
static const char help_24[] PROGMEM = "dac set <0-4095> / dac status (MCP4921, CS D53)";
static const char help_25[] PROGMEM = "display on / off / page <0-2> / refresh / status (SSD1306)";

static const char *const help_lines[] PROGMEM = {
    help_0,
    help_1,
    help_2,
    help_3,
    help_4,
    help_5,
    help_6,
    help_7,
    help_8,
    help_9,
    help_10,
    help_11,
    help_12,
    help_13,
    help_14,
    help_15,
    help_16,
    help_17,
    help_18,
    help_19,
    help_20,
    help_21,
    help_22,
    help_23,
    help_24,
    help_25
};

#define REPORT_HELP_COUNT (sizeof(help_lines) / sizeof(help_lines[0]))

static uint8_t help_index = REPORT_HELP_COUNT;
static bool status_pending = false;
static bool config_pending = false;
static bool nh3_pending = false;
static bool led_pending = false;
static bool adc_pending = false;
static bool pot_pending = false;
static bool light_pending = false;
static bool buzzer_pending = false;
static bool counter_pending = false;
static bool range_pending = false;
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
        usart0_send_string_P(PSTR("-"));
        number = -number;
    }
    usart0_send_uint32(number);
}

void report_update(uint32_t now)
{
    motor_state_t state = motor_get_state();

    if(state != last_state && report_room(REPORT_LINE_BYTES)){
        last_state = state;
        usart0_send_string_P(PSTR("["));
        usart0_send_uint32(now);
        usart0_send_string_P(PSTR(" ms] "));
        command_motor_send_state(state);
        usart0_send_string_P(PSTR(" "));
        usart0_send_uint32(motor_get_current_hz());
        usart0_send_string_P(PSTR(" Hz"));
        usart0_send_newline();
    }

    nh3_state_t nh3_state = nh3_get_state();

    if(nh3_state != last_nh3_state && report_room(REPORT_LINE_BYTES)){
        last_nh3_state = nh3_state;
        usart0_send_string_P(PSTR("["));
        usart0_send_uint32(now);
        usart0_send_string_P(PSTR(" ms] NH3 "));
        nh3_send_state(nh3_state);
        usart0_send_newline();
    }

    while(help_index < REPORT_HELP_COUNT){
        const char *line = (const char *)pgm_read_word(&help_lines[help_index]);

        if(!report_room(strlen_P(line) + 2)){
            break;
        }

        usart0_send_string_P(line);
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
        analog_send_status(now);
    }

    if(pot_pending && report_room(REPORT_POT_BYTES)){
        pot_pending = false;
        pot_send_filter_status(command_pot_config_channel());
    }

    if(light_pending && report_room(REPORT_LIGHT_BYTES)){
        light_pending = false;
        light_send_status(now);
    }

    if(buzzer_pending && report_room(REPORT_BUZZER_BYTES)){
        buzzer_pending = false;
        buzzer_send_status(now);
    }

    if(counter_pending && report_room(REPORT_COUNTER_BYTES)){
        counter_pending = false;
        counter_send_status(now);
    }

    if(counter_line_ready() && report_room(REPORT_COUNTER_LINE_BYTES)){
        counter_send_line(now);
    }

    if(range_alarm_event() && report_room(REPORT_RANGE_EVENT_BYTES)){
        range_send_alarm_event(now);
    }

    if(range_pending && report_room(REPORT_RANGE_BYTES)){
        range_pending = false;
        range_send_status(now);
    }

    if(scanner_head_pending() && report_room(REPORT_I2C_HEAD_BYTES)){
        scanner_send_head();
    }

    if(imu_event_pending() && report_room(REPORT_IMU_EVENT_BYTES)){
        imu_send_event();
    }

    if(imu_report_pending() && report_room(REPORT_IMU_BYTES)){
        imu_send_report();
    }

    if(loopback_event_pending() && report_room(REPORT_SPI_EVENT_BYTES)){
        loopback_send_event();
    }

    if(loopback_report_pending() && report_room(REPORT_SPI_LINE_BYTES)){
        loopback_send_report();
    }

    if(dac_event_pending() && report_room(REPORT_DAC_EVENT_BYTES)){
        dac_send_event();
    }

    if(dac_report_pending() && report_room(REPORT_DAC_LINE_BYTES)){
        dac_send_report();
    }

    if(display_event_pending() && report_room(REPORT_DISPLAY_EVENT_BYTES)){
        display_send_event();
    }

    if(display_report_pending() && report_room(REPORT_DISPLAY_LINE_BYTES)){
        display_send_report();
    }

    if(log_enabled && now - last_log >= REPORT_LOG_MS){
        last_log = now;

        if(report_room(REPORT_LOG_BYTES)){
            usart0_send_string_P(PSTR("LOG,"));
            usart0_send_uint32(now);
            usart0_send_string_P(PSTR(","));
            usart0_send_uint32(motor_get_current_hz());
            usart0_send_string_P(PSTR(","));
            report_send_int32(motor_get_position());
            usart0_send_string_P(PSTR(","));

            uint32_t ppb = 0;
            if(nh3_get_ppb(&ppb)){
                usart0_send_uint32(ppb);
            } else {
                usart0_send_string_P(PSTR("-"));
            }
            usart0_send_string_P(PSTR(","));

            for(uint8_t i = 0; i < 2; i++){
                if(pot_is_valid(i)){
                    usart0_send_uint32(pot_get_raw(i));
                } else {
                    usart0_send_string_P(PSTR("-"));
                }
                usart0_send_string_P(PSTR(","));
            }
            usart0_send_uint32(pwm4a_get_duty());

            pot_view_t view;
            if(pot_get_view(pot_get_source(), &view) && view.ready){
                usart0_send_string_P(PSTR(","));
                usart0_send_uint32(view.avg);
                usart0_send_string_P(PSTR(","));
                usart0_send_uint32(view.cal);
                usart0_send_string_P(PSTR(","));
                usart0_send_uint32(view.map);
                usart0_send_string_P(PSTR(","));
                usart0_send_uint32(view.hold);
            }
            usart0_send_newline();
        }
    }

    if(now - last_heartbeat >= REPORT_HEARTBEAT_MS && report_room(REPORT_STATUS_BYTES + REPORT_NH3_BRIEF_BYTES)){
        last_heartbeat = now;

        dropped_total += usart0_tx_take_dropped();

        usart0_send_string_P(PSTR("TIME: "));
        usart0_send_uint32(now / 1000);
        usart0_send_string_P(PSTR(" s, "));
        command_motor_send_status();
        nh3_send_brief(now);

        if(dropped_total != 0){
            usart0_send_string_P(PSTR("TX DROPPED: "));
            usart0_send_uint32(dropped_total);
            usart0_send_newline();
        }
    }

    uint16_t list_room = REPORT_I2C_LINE_BYTES;

    if(log_enabled){
        list_room += REPORT_LOG_BYTES;
    }

    if(!scanner_head_pending() && scanner_list_pending() && report_room(list_room)){
        scanner_send_list_line();
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

void report_request_pot(void)
{
    pot_pending = true;
}

void report_request_light(void)
{
    light_pending = true;
}

void report_request_buzzer(void)
{
    buzzer_pending = true;
}

void report_request_counter(void)
{
    counter_pending = true;
}

void report_request_range(void)
{
    range_pending = true;
}

void report_set_log(bool enable)
{
    log_enabled = enable;
}
