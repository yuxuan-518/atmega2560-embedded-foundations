#ifndef SSD1306_H
#define SSD1306_H

#include <stdint.h>
#include <stdbool.h>
#include "i2c.h"

#define SSD1306_WIDTH 128
#define SSD1306_HEIGHT 64
#define SSD1306_PAGES 8
#define SSD1306_BUFFER_SIZE 1024
#define SSD1306_ADDRESS_LOW 0x3C
#define SSD1306_ADDRESS_HIGH 0x3D
#define SSD1306_CONTROL_COMMAND 0x00
#define SSD1306_CONTROL_DATA 0x40
#define SSD1306_CHUNK (I2C_BUFFER_SIZE - 1)
#define SSD1306_DISPLAY_OFF 0xAE
#define SSD1306_DISPLAY_ON 0xAF
#define SSD1306_COLUMN_ADDRESS 0x21
#define SSD1306_PAGE_ADDRESS 0x22

typedef enum {
    SSD1306_IDLE,
    SSD1306_BUSY,
    SSD1306_OK,
    SSD1306_FAIL,
    SSD1306_ABORTED
} ssd1306_result_t;

typedef enum {
    SSD1306_JOB_NONE,
    SSD1306_JOB_INIT,
    SSD1306_JOB_COMMAND,
    SSD1306_JOB_WRITE
} ssd1306_job_t;

bool ssd1306_setup(uint8_t owner, uint8_t address);
bool ssd1306_address_valid(uint8_t address);

bool ssd1306_start_init(void);
bool ssd1306_start_command(uint8_t command);
bool ssd1306_start_write(const uint8_t *frame, uint8_t page_first, uint8_t page_last, uint8_t col_first, uint8_t col_last);
ssd1306_result_t ssd1306_update(uint32_t now);
void ssd1306_abort(void);

bool ssd1306_is_busy(void);
bool ssd1306_in_flight(void);
ssd1306_job_t ssd1306_get_job(void);
uint8_t ssd1306_get_address(void);
uint16_t ssd1306_get_sent(void);
uint16_t ssd1306_get_total(void);
uint32_t ssd1306_get_transactions(void);
i2c_result_t ssd1306_get_error(void);
i2c_phase_t ssd1306_get_error_phase(void);
uint8_t ssd1306_get_error_status(void);
uint8_t ssd1306_init_length(void);

#endif
