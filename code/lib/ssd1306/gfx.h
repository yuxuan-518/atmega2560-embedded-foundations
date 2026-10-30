#ifndef GFX_H
#define GFX_H

#include <stdint.h>
#include <stdbool.h>
#include "ssd1306.h"

#define GFX_SEGMENT_WIDTH 16
#define GFX_SEGMENTS (SSD1306_WIDTH / GFX_SEGMENT_WIDTH)
#define GFX_CHAR_WIDTH 6
#define GFX_TEXT_COLUMNS (SSD1306_WIDTH / GFX_CHAR_WIDTH)
#define GFX_INVERT 0x01
#define GFX_UNDERLINE 0x02

void gfx_init(void);
const uint8_t *gfx_frame(void);
uint8_t gfx_get(uint8_t page, uint8_t col);
void gfx_put(uint8_t page, uint8_t col, uint8_t value);
void gfx_clear(void);
void gfx_fill(uint8_t page, uint8_t col, uint8_t width, uint8_t value);
uint8_t gfx_char(uint8_t page, uint8_t col, char c, bool invert);
uint8_t gfx_text(uint8_t page, uint8_t col, const char *text, bool invert);
void gfx_row(uint8_t page, const char *text, uint8_t style);
void gfx_bar(uint8_t page, uint8_t col, uint8_t width, uint8_t filled, uint8_t style);

void gfx_pixel(uint8_t x, uint8_t y, bool on);
bool gfx_get_pixel(uint8_t x, uint8_t y);
void gfx_hline(uint8_t x, uint8_t y, uint8_t width, bool on);
void gfx_vline(uint8_t x, uint8_t y, uint8_t height, bool on);

void gfx_mark_all(void);
bool gfx_dirty_any(void);
bool gfx_dirty_all(void);
uint8_t gfx_dirty_mask(uint8_t page);
uint16_t gfx_dirty_bytes(void);
bool gfx_next_window(uint8_t *page_first, uint8_t *page_last, uint8_t *col_first, uint8_t *col_last);
void gfx_clean_window(uint8_t page_first, uint8_t page_last, uint8_t col_first, uint8_t col_last);

#endif
