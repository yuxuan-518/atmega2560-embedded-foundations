#include "mcp4921.h"

bool mcp4921_command(uint16_t value, bool buffered, bool gain_1x, bool active, uint8_t out[MCP4921_BYTES])
{
    if(value > MCP4921_MAX || out == 0){
        return false;
    }

    uint16_t word = value;

    if(buffered){
        word |= MCP4921_BUF;
    }
    if(gain_1x){
        word |= MCP4921_GA_1X;
    }
    if(active){
        word |= MCP4921_ACTIVE;
    }

    out[0] = (uint8_t)(word >> 8);
    out[1] = (uint8_t)word;
    return true;
}

uint32_t mcp4921_microvolts(uint16_t value, uint32_t vref_mv, bool gain_1x)
{
    if(value > MCP4921_MAX){
        value = MCP4921_MAX;
    }

    uint32_t uv = ((uint32_t)value * vref_mv * 125UL + 256UL) / 512UL;

    if(!gain_1x){
        uv *= 2;
    }
    return uv;
}

bool mcp4921_mode_ok(uint8_t mode)
{
    return mode == 0 || mode == 3;
}
