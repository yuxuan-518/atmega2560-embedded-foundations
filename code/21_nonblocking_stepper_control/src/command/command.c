#include "command/command.h"
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

    // —————————————————————————————————————— 归零 ————————————————————————————————————————
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
