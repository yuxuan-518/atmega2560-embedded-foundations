#ifndef NH3_PROTOCOL_H
#define NH3_PROTOCOL_H

#include <stdint.h>
#include <stdbool.h>
#include "frame.h"
#include "config.h"

#define NH3_PROTOCOL_NONE 0
#define NH3_PROTOCOL_TEST 1
#define NH3_PROTOCOL_ZE801 2
#define NH3_PROTOCOL_ZE03 3

#ifndef NH3_PROTOCOL
#define NH3_PROTOCOL NH3_PROTOCOL_ZE03
#endif

#define NH3_RANGE_PPB (CONFIG_NH3_RANGE_PPM * 1000UL)

typedef enum {
    NH3_DECODE_OK,
    NH3_DECODE_WARMUP,
    NH3_DECODE_BAD_FIELD,
    NH3_DECODE_RANGE
} nh3_decode_t;

const frame_format_t *nh3_protocol_format(void);
const char *nh3_protocol_name(void);
nh3_decode_t nh3_protocol_decode(const uint8_t frame[], uint32_t *ppb);
uint8_t nh3_protocol_active_command(const uint8_t **command);

#endif
