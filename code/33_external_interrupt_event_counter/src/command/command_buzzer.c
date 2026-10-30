#include "command/command_buzzer.h"
#include "command.h"
#include "config.h"
#include "buzzer.h"
#include "report.h"
#include "usart.h"

#include <string.h>
#include <avr/pgmspace.h>

static void command_buzzer_ok(const char *text)
{
    usart0_send_string_P(PSTR("OK buzzer "));
    usart0_send_string_P(text);
    usart0_send_newline();
}

static void command_buzzer_warning(const char *text)
{
    usart0_send_string_P(PSTR("WARNING "));
    usart0_send_string_P(text);
    usart0_send_newline();
}

void command_buzzer_execute(char *args[], uint8_t count, uint32_t now)
{
    int32_t hz = 0;
    int32_t ms = 0;

    if(count == 4 && strcmp(args[1], "tone") == 0){
        if(!command_parse_int32(args[2], &hz) || !command_parse_int32(args[3], &ms)){
            command_buzzer_warning(PSTR("use buzzer tone <Hz> <ms>"));
            return;
        }

        if(hz != 0 && (hz < (int32_t)CONFIG_BUZZER_MIN_HZ || hz > (int32_t)CONFIG_BUZZER_MAX_HZ)){
            command_buzzer_warning(PSTR("buzzer Hz must be 0 (rest) or 100-5000"));
            return;
        }

        if(ms < (int32_t)CONFIG_BUZZER_MIN_MS || ms > (int32_t)CONFIG_BUZZER_MAX_MS){
            command_buzzer_warning(PSTR("buzzer ms must be 1-10000"));
            return;
        }

        if(!buzzer_tone(hz, ms, now)){
            command_buzzer_warning(PSTR("buzzer tone rejected"));
            return;
        }

        if(hz == 0){
            command_buzzer_ok(PSTR("rest"));
        } else {
            command_buzzer_ok(PSTR("tone"));
        }
    }
    else if(count == 3 && strcmp(args[1], "play") == 0 && strcmp(args[2], "scale") == 0){
        buzzer_play_scale(now);
        command_buzzer_ok(PSTR("play scale"));
    }
    else if(count == 3 && strcmp(args[1], "play") == 0 && strcmp(args[2], "melody") == 0){
        buzzer_play_melody(now);
        command_buzzer_ok(PSTR("play melody"));
    }
    else if(count == 2 && strcmp(args[1], "stop") == 0){
        buzzer_stop();
        command_buzzer_ok(PSTR("stop"));
    }
    else if(count == 2 && strcmp(args[1], "status") == 0){
        report_request_buzzer();
    }
    else {
        command_buzzer_warning(PSTR("use buzzer tone <Hz> <ms> / play scale|melody / stop / status"));
    }
}
