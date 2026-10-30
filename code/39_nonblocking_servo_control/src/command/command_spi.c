#include "command/command_spi.h"
#include "command.h"
#include "loopback.h"
#include "usart.h"

#include <string.h>
#include <avr/pgmspace.h>

static void command_spi_ok(const char *text)
{
    usart0_send_string_P(PSTR("OK spi "));
    usart0_send_string_P(text);
}

static void command_spi_warning(const char *text)
{
    usart0_send_string_P(PSTR("WARNING "));
    usart0_send_string_P(text);
    usart0_send_newline();
}

static void command_spi_settings(void)
{
    spi_settings_t settings;

    loopback_get_settings(&settings);
    usart0_send_string_P(PSTR("mode "));
    usart0_send_uint32(settings.mode);
    usart0_send_string_P(PSTR(", "));
    usart0_send_uint32(spi_settings_hz(&settings));
    usart0_send_string_P(PSTR(" Hz (loopback; DAC fixed)"));
    usart0_send_newline();
}

static void command_spi_config(loop_cfg_t result, const char *bad)
{
    if(result == LOOP_CFG_OK){
        command_spi_ok(PSTR(""));
        command_spi_settings();
    } else if(result == LOOP_CFG_BUSY){
        command_spi_warning(PSTR("spi config only when idle and CS released, use spi stop first"));
    } else {
        command_spi_warning(bad);
    }
}

void command_spi_execute(char *args[], uint8_t count, uint32_t now)
{
    int32_t value = 0;

    if(count == 2 && strcmp(args[1], "loopback") == 0){
        loop_start_t result = loopback_start(now);

        if(result == LOOP_START_OK){
            command_spi_ok(PSTR("loopback 6 patterns x 256 B, "));
            command_spi_settings();
        } else if(result == LOOP_START_STOPPING){
            command_spi_warning(PSTR("loopback still stopping, try again"));
        } else {
            command_spi_ok(PSTR("loopback already running"));
            usart0_send_newline();
        }
    }
    else if(count == 3 && strcmp(args[1], "mode") == 0){
        if(!command_parse_int32(args[2], &value) || value < 0 || value > 3){
            command_spi_warning(PSTR("use spi mode <0-3>"));
            return;
        }
        command_spi_config(loopback_set_mode((uint8_t)value), PSTR("use spi mode <0-3>"));
    }
    else if(count == 3 && strcmp(args[1], "clock") == 0){
        if(!command_parse_int32(args[2], &value) || value < (int32_t)SPI_MIN_HZ || value > (int32_t)SPI_MAX_HZ){
            command_spi_warning(PSTR("spi clock must be 125000-8000000 Hz"));
            return;
        }
        command_spi_config(loopback_set_clock((uint32_t)value), PSTR("spi clock must be 125000-8000000 Hz"));
    }
    else if(count == 2 && strcmp(args[1], "stop") == 0){
        if(loopback_stop()){
            command_spi_ok(PSTR("stop, loopback cancelled"));
        } else if(loopback_stopping()){
            command_spi_ok(PSTR("stop, already stopping"));
        } else {
            command_spi_ok(PSTR("stop, no loopback running"));
        }
        usart0_send_newline();
    }
    else if(count == 2 && strcmp(args[1], "status") == 0){
        loopback_request_report();
    }
    else {
        command_spi_warning(PSTR("use spi loopback / mode <0-3> / clock <Hz> / stop / status"));
    }
}
