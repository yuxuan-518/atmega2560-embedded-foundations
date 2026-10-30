#ifndef FRAME_H
#define FRAME_H

#include <stdint.h>
#include <stdbool.h>

#define FRAME_MAX_LEN 32
#define FRAME_MAX_HEADER 4

typedef enum {
    FRAME_CHECK_NONE,
    FRAME_CHECK_SUM8,
    FRAME_CHECK_SUM8_NEG,
    FRAME_CHECK_XOR8,
    FRAME_CHECK_CRC16_MODBUS
} frame_check_t;

typedef struct {
    uint8_t header[FRAME_MAX_HEADER];
    uint8_t header_len;
    uint8_t length;
    frame_check_t check;
    uint8_t check_from;
    uint8_t check_to;
    uint16_t gap_ms;
} frame_format_t;

typedef enum {
    FRAME_NONE,
    FRAME_OK,
    FRAME_BAD_CHECK
} frame_result_t;

typedef struct {
    const frame_format_t *format;
    uint8_t buffer[FRAME_MAX_LEN];
    uint8_t frame[FRAME_MAX_LEN];
    uint8_t index;
    uint32_t last_byte_ms;
    uint32_t ok;
    uint32_t bad_check;
    uint32_t incomplete;
    uint32_t skipped;
} frame_parser_t;

bool frame_format_valid(const frame_format_t *format);
void frame_init(frame_parser_t *parser, const frame_format_t *format);
frame_result_t frame_push(frame_parser_t *parser, uint8_t data, uint32_t now);
void frame_check_gap(frame_parser_t *parser, uint32_t now);
void frame_drop_partial(frame_parser_t *parser);

#endif
