#include "nh3_protocol.h"

#if NH3_PROTOCOL == NH3_PROTOCOL_ZE03

#define ZE03_GAS_NH3 0x02
#define ZE03_MAX_DECIMALS 2

static const frame_format_t nh3_format = {
    {0xFF, 0x86, 0x00, 0x00},
    2,
    9,
    FRAME_CHECK_SUM8_NEG,
    1,
    7,
    20
};

static const uint8_t ze03_active_mode[9] = {
    0xFF, 0x01, 0x78, 0x03, 0x00, 0x00, 0x00, 0x00, 0x84
};

const frame_format_t *nh3_protocol_format(void)
{
    return &nh3_format;
}

const char *nh3_protocol_name(void)
{
    return "ZE03 active upload";
}

nh3_decode_t nh3_protocol_decode(const uint8_t frame[], uint32_t *ppb)
{
    if(frame[4] != ZE03_GAS_NH3){
        return NH3_DECODE_BAD_FIELD;
    }

    uint8_t decimals = frame[5];

    if(decimals > ZE03_MAX_DECIMALS){
        return NH3_DECODE_BAD_FIELD;
    }

    uint32_t raw = ((uint32_t)frame[2] << 8) | (uint32_t)frame[3];
    uint32_t scale = 1000;

    for(uint8_t i = 0; i < decimals; i++){
        scale /= 10;
    }

    uint32_t value = raw * scale;

    if(value > NH3_RANGE_PPB){
        return NH3_DECODE_RANGE;
    }

    *ppb = value;
    return NH3_DECODE_OK;
}

uint8_t nh3_protocol_active_command(const uint8_t **command)
{
    *command = ze03_active_mode;
    return sizeof(ze03_active_mode);
}

#elif NH3_PROTOCOL == NH3_PROTOCOL_TEST

static const frame_format_t nh3_format = {
    {0xA5, 0x5A, 0x00, 0x00},
    2,
    8,
    FRAME_CHECK_SUM8,
    2,
    6,
    20
};

const frame_format_t *nh3_protocol_format(void)
{
    return &nh3_format;
}

const char *nh3_protocol_name(void)
{
    return "TEST (not a real sensor)";
}

nh3_decode_t nh3_protocol_decode(const uint8_t frame[], uint32_t *ppb)
{
    if(frame[2] != 0x01){
        return NH3_DECODE_BAD_FIELD;
    }

    uint32_t value = ((uint32_t)frame[4] << 16) |
                     ((uint32_t)frame[5] << 8) |
                     (uint32_t)frame[6];

    if(value > NH3_RANGE_PPB){
        return NH3_DECODE_RANGE;
    }

    *ppb = value;

    if(frame[3] == 0x01){
        return NH3_DECODE_WARMUP;
    }

    if(frame[3] != 0x00){
        return NH3_DECODE_BAD_FIELD;
    }

    return NH3_DECODE_OK;
}

uint8_t nh3_protocol_active_command(const uint8_t **command)
{
    *command = 0;
    return 0;
}

#elif NH3_PROTOCOL == NH3_PROTOCOL_ZE801

#error "ZE801 protocol not confirmed: fill nh3_format and nh3_protocol_decode from the official protocol document"

#else

const frame_format_t *nh3_protocol_format(void)
{
    return 0;
}

const char *nh3_protocol_name(void)
{
    return "NONE (raw only)";
}

nh3_decode_t nh3_protocol_decode(const uint8_t frame[], uint32_t *ppb)
{
    (void)frame;
    (void)ppb;
    return NH3_DECODE_BAD_FIELD;
}

uint8_t nh3_protocol_active_command(const uint8_t **command)
{
    *command = 0;
    return 0;
}

#endif
