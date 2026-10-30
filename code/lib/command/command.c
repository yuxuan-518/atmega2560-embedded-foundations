#include "command.h"
#include "usart.h"

#include <string.h>

bool command_receive(char line[COMMAND_LEN])
{
    static char buffer[COMMAND_LEN] = "";
    static uint8_t index_read = 0;
    static bool too_long_status = false;

    if(!usart0_rx_ready()){
        return false;
    }

    uint8_t read = usart0_rx_read();

    if(read == '\x08' || read == 0x7F){
        if(index_read > 0){
            index_read--;
            buffer[index_read] = '\0';
        }
        return false;
    }

    if(read == '\r'){
        return false;
    }

    if(read != '\n'){
        if(index_read < COMMAND_LEN - 1){
            buffer[index_read] = read;
            index_read++;
            buffer[index_read] = '\0';
        } else {
            too_long_status = true;
        }
        return false;
    }

    bool command_ready = false;

    if(too_long_status == true){
        usart0_send_string("WARNING command too long");
        usart0_send_newline();
    } else if(index_read > 0){
        usart0_send_string("> ");
        usart0_send_string(buffer);
        usart0_send_newline();

        strcpy(line, buffer);
        command_ready = true;
    }

    buffer[0] = '\0';
    index_read = 0;
    too_long_status = false;

    return command_ready;
}

uint32_t command_parse_uint32(const char text[])
{
    uint32_t number = 0;
    uint8_t index = 0;

    if(text[0] == '\0'){
        return 0;
    }

    while(text[index] != '\0'){
        if(text[index] < '0' || text[index] > '9'){
            return 0;
        }

        number = number * 10 + (text[index] - '0');

        if(number > 1000000UL){
            return 0;
        }

        index++;
    }

    return number;
}

bool command_parse_int32(const char text[], int32_t *value)
{
    uint8_t index = 0;
    bool negative = false;
    uint32_t number = 0;

    if(text[0] == '-'){
        negative = true;
        index = 1;
    } else if(text[0] == '+'){
        index = 1;
    }

    if(text[index] == '\0'){
        return false;
    }

    while(text[index] != '\0'){
        if(text[index] < '0' || text[index] > '9'){
            return false;
        }

        uint8_t digit = text[index] - '0';

        if(number > (COMMAND_INT_MAX - digit) / 10){
            return false;
        }

        number = number * 10 + digit;
        index++;
    }

    if(negative){
        *value = -(int32_t)number;
    } else {
        *value = (int32_t)number;
    }

    return true;
}

uint8_t command_split(char line[], char *args[], uint8_t max_args)
{
    uint8_t count = 0;
    uint8_t index = 0;

    while(line[index] != '\0'){
        if(line[index] == ' '){
            line[index] = '\0';
            index++;
            continue;
        }

        if(count < max_args){
            args[count] = &line[index];
        }
        count++;

        while(line[index] != ' ' && line[index] != '\0'){
            index++;
        }
    }

    return count;
}
