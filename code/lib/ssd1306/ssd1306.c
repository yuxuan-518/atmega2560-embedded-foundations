#include "ssd1306.h"
#include <avr/pgmspace.h>

static const uint8_t ssd1306_init_table[] PROGMEM = {
    15, SSD1306_CONTROL_COMMAND, 0xAE, 0xD5, 0x80, 0xA8, 0x3F, 0xD3, 0x00, 0x40, 0x8D, 0x14, 0x20, 0x00, 0xA1, 0xC8,
    12, SSD1306_CONTROL_COMMAND, 0xDA, 0x12, 0x81, 0xCF, 0xD9, 0xF1, 0xDB, 0x20, 0xA4, 0xA6, 0x2E,
    0
};

static uint8_t ssd1306_owner = I2C_OWNER_ANY;
static uint8_t ssd1306_address = SSD1306_ADDRESS_LOW;
static ssd1306_job_t ssd1306_job = SSD1306_JOB_NONE;
static bool ssd1306_flight = false;
static bool ssd1306_abort_request = false;
static uint8_t ssd1306_command = 0;
static uint16_t ssd1306_init_pos = 0;
static uint16_t ssd1306_next_init = 0;
static const uint8_t *ssd1306_frame = 0;
static uint8_t ssd1306_page_first = 0;
static uint8_t ssd1306_col_first = 0;
static uint8_t ssd1306_width = 0;
static uint8_t ssd1306_row = 0;
static uint8_t ssd1306_col = 0;
static bool ssd1306_window_sent = false;
static uint16_t ssd1306_total = 0;
static uint16_t ssd1306_sent = 0;
static uint8_t ssd1306_chunk = 0;
static uint32_t ssd1306_transactions = 0;
static i2c_result_t ssd1306_error = I2C_NONE;
static i2c_phase_t ssd1306_error_phase = I2C_PHASE_IDLE;
static uint8_t ssd1306_error_status = I2C_STATUS_NONE;

bool ssd1306_address_valid(uint8_t address)
{
    return address == SSD1306_ADDRESS_LOW || address == SSD1306_ADDRESS_HIGH;
}

bool ssd1306_setup(uint8_t owner, uint8_t address)
{
    if(owner == I2C_OWNER_ANY || !ssd1306_address_valid(address) || ssd1306_job != SSD1306_JOB_NONE){
        return false;
    }

    ssd1306_owner = owner;
    ssd1306_address = address;
    return true;
}

static bool ssd1306_begin(ssd1306_job_t job)
{
    if(ssd1306_job != SSD1306_JOB_NONE || ssd1306_flight){
        return false;
    }

    ssd1306_job = job;
    ssd1306_abort_request = false;
    ssd1306_sent = 0;
    ssd1306_total = 0;
    return true;
}

bool ssd1306_start_init(void)
{
    if(!ssd1306_begin(SSD1306_JOB_INIT)){
        return false;
    }

    ssd1306_init_pos = 0;
    ssd1306_total = ssd1306_init_length();
    return true;
}

bool ssd1306_start_command(uint8_t command)
{
    if(!ssd1306_begin(SSD1306_JOB_COMMAND)){
        return false;
    }

    ssd1306_command = command;
    ssd1306_total = 1;
    return true;
}

bool ssd1306_start_write(const uint8_t *frame, uint8_t page_first, uint8_t page_last, uint8_t col_first, uint8_t col_last)
{
    if(frame == 0 || page_first > page_last || page_last >= SSD1306_PAGES || col_first > col_last || col_last >= SSD1306_WIDTH){
        return false;
    }

    if(!ssd1306_begin(SSD1306_JOB_WRITE)){
        return false;
    }

    ssd1306_frame = frame;
    ssd1306_page_first = page_first;
    ssd1306_col_first = col_first;
    ssd1306_width = (uint8_t)(col_last - col_first + 1);
    ssd1306_total = (uint16_t)ssd1306_width * (uint16_t)(page_last - page_first + 1);
    ssd1306_window_sent = false;
    ssd1306_row = 0;
    ssd1306_col = 0;
    return true;
}

static uint8_t ssd1306_build(uint8_t *out)
{
    if(ssd1306_job == SSD1306_JOB_INIT){
        uint8_t length = pgm_read_byte(&ssd1306_init_table[ssd1306_init_pos]);

        for(uint8_t i = 0; i < length; i++){
            out[i] = pgm_read_byte(&ssd1306_init_table[ssd1306_init_pos + 1 + i]);
        }
        ssd1306_next_init = ssd1306_init_pos + 1 + length;
        ssd1306_chunk = (uint8_t)(length - 1);
        return length;
    }

    if(ssd1306_job == SSD1306_JOB_COMMAND){
        out[0] = SSD1306_CONTROL_COMMAND;
        out[1] = ssd1306_command;
        ssd1306_chunk = 1;
        return 2;
    }

    if(!ssd1306_window_sent){
        out[0] = SSD1306_CONTROL_COMMAND;
        out[1] = SSD1306_COLUMN_ADDRESS;
        out[2] = ssd1306_col_first;
        out[3] = (uint8_t)(ssd1306_col_first + ssd1306_width - 1);
        out[4] = SSD1306_PAGE_ADDRESS;
        out[5] = ssd1306_page_first;
        out[6] = (uint8_t)(ssd1306_page_first + ssd1306_total / ssd1306_width - 1);
        ssd1306_chunk = 0;
        return 7;
    }

    uint16_t left = ssd1306_total - ssd1306_sent;
    uint8_t count = left > SSD1306_CHUNK ? SSD1306_CHUNK : (uint8_t)left;

    uint8_t row = ssd1306_row;
    uint8_t col = ssd1306_col;

    out[0] = SSD1306_CONTROL_DATA;

    for(uint8_t i = 0; i < count; i++){
        out[1 + i] = ssd1306_frame[(uint16_t)(ssd1306_page_first + row) * SSD1306_WIDTH + ssd1306_col_first + col];
        col++;

        if(col >= ssd1306_width){
            col = 0;
            row++;
        }
    }

    ssd1306_chunk = count;
    return (uint8_t)(count + 1);
}

static bool ssd1306_job_done(void)
{
    if(ssd1306_job == SSD1306_JOB_INIT){
        ssd1306_init_pos = ssd1306_next_init;
        ssd1306_sent += ssd1306_chunk;
        return pgm_read_byte(&ssd1306_init_table[ssd1306_init_pos]) == 0;
    }

    if(ssd1306_job == SSD1306_JOB_COMMAND){
        ssd1306_sent = 1;
        return true;
    }

    if(!ssd1306_window_sent){
        ssd1306_window_sent = true;
        return false;
    }

    ssd1306_sent += ssd1306_chunk;
    ssd1306_col += ssd1306_chunk;

    while(ssd1306_col >= ssd1306_width){
        ssd1306_col -= ssd1306_width;
        ssd1306_row++;
    }
    return ssd1306_sent >= ssd1306_total;
}

ssd1306_result_t ssd1306_update(uint32_t now)
{
    if(ssd1306_flight){
        i2c_result_t result = i2c_update(ssd1306_owner, now);

        if(result == I2C_BUSY || result == I2C_NONE){
            return SSD1306_BUSY;
        }

        ssd1306_flight = false;
        ssd1306_transactions++;

        if(result != I2C_OK){
            ssd1306_error = result;
            ssd1306_error_phase = i2c_get_fail_phase();
            ssd1306_error_status = i2c_get_status();
            ssd1306_job = SSD1306_JOB_NONE;
            return SSD1306_FAIL;
        }

        if(ssd1306_job_done()){
            ssd1306_job = SSD1306_JOB_NONE;
            return SSD1306_OK;
        }

        if(ssd1306_abort_request){
            ssd1306_job = SSD1306_JOB_NONE;
            return SSD1306_ABORTED;
        }
        return SSD1306_BUSY;
    }

    if(ssd1306_job == SSD1306_JOB_NONE){
        return SSD1306_IDLE;
    }

    if(ssd1306_abort_request){
        ssd1306_job = SSD1306_JOB_NONE;
        return SSD1306_ABORTED;
    }

    if(i2c_is_busy()){
        return SSD1306_BUSY;
    }

    uint8_t data[I2C_BUFFER_SIZE];
    uint8_t length = ssd1306_build(data);

    if(i2c_write(ssd1306_owner, ssd1306_address, data, length, now)){
        ssd1306_flight = true;
    }
    return SSD1306_BUSY;
}

void ssd1306_abort(void)
{
    if(ssd1306_job != SSD1306_JOB_NONE){
        ssd1306_abort_request = true;
    }
}

bool ssd1306_is_busy(void)
{
    return ssd1306_job != SSD1306_JOB_NONE || ssd1306_flight;
}

bool ssd1306_in_flight(void)
{
    return ssd1306_flight;
}

ssd1306_job_t ssd1306_get_job(void)
{
    return ssd1306_job;
}

uint8_t ssd1306_get_address(void)
{
    return ssd1306_address;
}

uint16_t ssd1306_get_sent(void)
{
    return ssd1306_sent;
}

uint16_t ssd1306_get_total(void)
{
    return ssd1306_total;
}

uint32_t ssd1306_get_transactions(void)
{
    return ssd1306_transactions;
}

i2c_result_t ssd1306_get_error(void)
{
    return ssd1306_error;
}

i2c_phase_t ssd1306_get_error_phase(void)
{
    return ssd1306_error_phase;
}

uint8_t ssd1306_get_error_status(void)
{
    return ssd1306_error_status;
}

uint8_t ssd1306_init_length(void)
{
    uint16_t pos = 0;
    uint8_t total = 0;
    uint8_t length = pgm_read_byte(&ssd1306_init_table[0]);

    while(length != 0){
        total = (uint8_t)(total + length - 1);
        pos = pos + 1 + length;
        length = pgm_read_byte(&ssd1306_init_table[pos]);
    }
    return total;
}
