#include "frame.h"

#include <string.h>

static uint16_t frame_crc16_modbus(const uint8_t data[], uint8_t from, uint8_t to)
{
    uint16_t crc = 0xFFFF;

    for(uint8_t i = from; i <= to; i++){
        crc ^= data[i];

        for(uint8_t bit = 0; bit < 8; bit++){
            if(crc & 0x0001){
                crc = (crc >> 1) ^ 0xA001;
            } else {
                crc >>= 1;
            }
        }
    }

    return crc;
}

static bool frame_check_ok(const frame_format_t *f, const uint8_t data[])
{
    uint8_t value = 0;

    switch(f->check){
        case FRAME_CHECK_NONE:
            return true;

        case FRAME_CHECK_SUM8:
            for(uint8_t i = f->check_from; i <= f->check_to; i++){
                value += data[i];
            }
            return value == data[f->length - 1];

        case FRAME_CHECK_SUM8_NEG:
            for(uint8_t i = f->check_from; i <= f->check_to; i++){
                value += data[i];
            }
            value = (uint8_t)(~value) + 1;
            return value == data[f->length - 1];

        case FRAME_CHECK_XOR8:
            for(uint8_t i = f->check_from; i <= f->check_to; i++){
                value ^= data[i];
            }
            return value == data[f->length - 1];

        case FRAME_CHECK_CRC16_MODBUS: {
            uint16_t crc = frame_crc16_modbus(data, f->check_from, f->check_to);
            uint16_t got = data[f->length - 2] | ((uint16_t)data[f->length - 1] << 8);
            return crc == got;
        }
    }

    return false;
}

static bool frame_prefix_matches(const frame_parser_t *p, uint8_t start)
{
    const frame_format_t *f = p->format;
    uint8_t available = p->index - start;

    if(available > f->header_len){
        available = f->header_len;
    }

    for(uint8_t i = 0; i < available; i++){
        if(p->buffer[start + i] != f->header[i]){
            return false;
        }
    }

    return true;
}

static void frame_resync(frame_parser_t *p, uint8_t start)
{
    for(uint8_t k = start; k < p->index; k++){
        if(frame_prefix_matches(p, k)){
            memmove(p->buffer, &p->buffer[k], p->index - k);
            p->index -= k;
            p->skipped += k;
            return;
        }
    }

    p->skipped += p->index;
    p->index = 0;
}

bool frame_format_valid(const frame_format_t *f)
{
    if(f == 0){
        return false;
    }

    if(f->header_len == 0 || f->header_len > FRAME_MAX_HEADER){
        return false;
    }

    if(f->length <= f->header_len || f->length > FRAME_MAX_LEN){
        return false;
    }

    if(f->check == FRAME_CHECK_NONE){
        return true;
    }

    uint8_t check_bytes = 1;
    if(f->check == FRAME_CHECK_CRC16_MODBUS){
        check_bytes = 2;
    }

    if(f->length < check_bytes + 1){
        return false;
    }

    return f->check_from <= f->check_to && f->check_to < f->length - check_bytes;
}

void frame_init(frame_parser_t *p, const frame_format_t *format)
{
    memset(p, 0, sizeof(frame_parser_t));
    p->format = format;
}

void frame_drop_partial(frame_parser_t *p)
{
    if(p->index > 0){
        p->incomplete++;
        p->skipped += p->index;
        p->index = 0;
    }
}

void frame_check_gap(frame_parser_t *p, uint32_t now)
{
    if(p->index > 0 && now - p->last_byte_ms > p->format->gap_ms){
        frame_drop_partial(p);
    }
}

frame_result_t frame_push(frame_parser_t *p, uint8_t data, uint32_t now)
{
    const frame_format_t *f = p->format;

    frame_check_gap(p, now);
    p->last_byte_ms = now;

    p->buffer[p->index] = data;
    p->index++;

    if(p->index <= f->header_len){
        if(data != f->header[p->index - 1]){
            frame_resync(p, 1);
        }
        return FRAME_NONE;
    }

    if(p->index < f->length){
        return FRAME_NONE;
    }

    memcpy(p->frame, p->buffer, f->length);

    if(frame_check_ok(f, p->buffer)){
        p->ok++;
        p->index = 0;
        return FRAME_OK;
    }

    p->bad_check++;
    frame_resync(p, 1);
    return FRAME_BAD_CHECK;
}
