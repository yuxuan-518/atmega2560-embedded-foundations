#include "gfx.h"
#include "font5x7.h"
#include <avr/pgmspace.h>

static uint8_t gfx_buffer[SSD1306_BUFFER_SIZE];
static uint8_t gfx_dirty[SSD1306_PAGES];

void gfx_init(void)
{
    for(uint16_t i = 0; i < SSD1306_BUFFER_SIZE; i++){
        gfx_buffer[i] = 0;
    }
    gfx_mark_all();
}

const uint8_t *gfx_frame(void)
{
    return gfx_buffer;
}

uint8_t gfx_get(uint8_t page, uint8_t col)
{
    if(page >= SSD1306_PAGES || col >= SSD1306_WIDTH){
        return 0;
    }
    return gfx_buffer[(uint16_t)page * SSD1306_WIDTH + col];
}

void gfx_put(uint8_t page, uint8_t col, uint8_t value)
{
    if(page >= SSD1306_PAGES || col >= SSD1306_WIDTH){
        return;
    }

    uint8_t *cell = &gfx_buffer[(uint16_t)page * SSD1306_WIDTH + col];

    if(*cell != value){
        *cell = value;
        gfx_dirty[page] |= (uint8_t)(1 << (col / GFX_SEGMENT_WIDTH));
    }
}

void gfx_clear(void)
{
    for(uint8_t page = 0; page < SSD1306_PAGES; page++){
        gfx_fill(page, 0, SSD1306_WIDTH, 0);
    }
}

void gfx_fill(uint8_t page, uint8_t col, uint8_t width, uint8_t value)
{
    if(page >= SSD1306_PAGES || col >= SSD1306_WIDTH){
        return;
    }

    uint8_t room = (uint8_t)(SSD1306_WIDTH - col);

    if(width > room){
        width = room;
    }

    for(uint8_t i = 0; i < width; i++){
        gfx_put(page, (uint8_t)(col + i), value);
    }
}

uint8_t gfx_char(uint8_t page, uint8_t col, char c, bool invert)
{
    if(page >= SSD1306_PAGES || col >= SSD1306_WIDTH){
        return SSD1306_WIDTH;
    }

    uint8_t mask = invert ? 0xFF : 0x00;
    uint8_t room = (uint8_t)(SSD1306_WIDTH - col);
    uint8_t count = room < GFX_CHAR_WIDTH ? room : GFX_CHAR_WIDTH;

    for(uint8_t i = 0; i < count; i++){
        gfx_put(page, (uint8_t)(col + i), (uint8_t)(font5x7_column(c, i) ^ mask));
    }
    return (uint8_t)(col + count);
}

uint8_t gfx_text(uint8_t page, uint8_t col, const char *text, bool invert)
{
    if(page >= SSD1306_PAGES || col >= SSD1306_WIDTH){
        return SSD1306_WIDTH;
    }

    while(*text != '\0' && col <= SSD1306_WIDTH - GFX_CHAR_WIDTH){
        col = gfx_char(page, col, *text, invert);
        text++;
    }
    return col;
}

static uint8_t gfx_style(uint8_t value, uint8_t style)
{
    if(style & GFX_INVERT){
        value ^= 0xFF;
    }
    if(style & GFX_UNDERLINE){
        value |= 0x80;
    }
    return value;
}

void gfx_row(uint8_t page, const char *text, uint8_t style)
{
    if(page >= SSD1306_PAGES){
        return;
    }

    uint8_t *cell = &gfx_buffer[(uint16_t)page * SSD1306_WIDTH];
    uint8_t flip = (style & GFX_INVERT) ? 0xFF : 0x00;
    uint8_t line = (style & GFX_UNDERLINE) ? 0x80 : 0x00;
    uint8_t blank = (uint8_t)(flip | line);
    uint8_t dirty = 0;
    uint8_t bit = 1;
    uint8_t left = GFX_SEGMENT_WIDTH;
    const uint8_t *glyph = 0;
    uint8_t k = GFX_CHAR_WIDTH;
    uint8_t chars = 0;

    for(uint8_t col = 0; col < SSD1306_WIDTH; col++){
        uint8_t value = blank;

        if(k == GFX_CHAR_WIDTH){
            k = 0;

            if(chars < GFX_TEXT_COLUMNS){
                char c = ' ';

                if(*text != '\0'){
                    c = *text;
                    text++;
                }
                glyph = font5x7_glyph(c);
                chars++;
            } else {
                glyph = 0;
            }
        }

        if(glyph != 0 && k < FONT5X7_WIDTH){
            value = (uint8_t)((pgm_read_byte(glyph + k) ^ flip) | line);
        }
        k++;

        if(*cell != value){
            *cell = value;
            dirty |= bit;
        }
        cell++;
        left--;

        if(left == 0){
            left = GFX_SEGMENT_WIDTH;
            bit = (uint8_t)(bit << 1);
        }
    }

    gfx_dirty[page] |= dirty;
}

void gfx_bar(uint8_t page, uint8_t col, uint8_t width, uint8_t filled, uint8_t style)
{
    if(page >= SSD1306_PAGES || width < 2 || col >= SSD1306_WIDTH || width > SSD1306_WIDTH - col){
        return;
    }

    uint8_t inner = (uint8_t)(width - 2);
    uint8_t *cell = &gfx_buffer[(uint16_t)page * SSD1306_WIDTH + col];
    uint8_t dirty = 0;

    if(filled > inner){
        filled = inner;
    }

    for(uint8_t i = 0; i < width; i++){
        uint8_t value = 0x42;

        if(i == 0 || i == width - 1 || i <= filled){
            value = 0x7E;
        }

        value = gfx_style(value, style);

        if(*cell != value){
            *cell = value;
            dirty |= (uint8_t)(1 << ((col + i) >> 4));
        }
        cell++;
    }

    gfx_dirty[page] |= dirty;
}

void gfx_pixel(uint8_t x, uint8_t y, bool on)
{
    if(x >= SSD1306_WIDTH || y >= SSD1306_HEIGHT){
        return;
    }

    uint8_t page = y >> 3;
    uint8_t bit = (uint8_t)(1 << (y & 7));
    uint8_t value = gfx_get(page, x);

    if(on){
        value |= bit;
    } else {
        value &= (uint8_t)~bit;
    }
    gfx_put(page, x, value);
}

bool gfx_get_pixel(uint8_t x, uint8_t y)
{
    if(x >= SSD1306_WIDTH || y >= SSD1306_HEIGHT){
        return false;
    }
    return (gfx_get(y >> 3, x) & (1 << (y & 7))) != 0;
}

void gfx_hline(uint8_t x, uint8_t y, uint8_t width, bool on)
{
    if(x >= SSD1306_WIDTH || y >= SSD1306_HEIGHT){
        return;
    }

    for(uint8_t i = 0; i < width && i < SSD1306_WIDTH - x; i++){
        gfx_pixel((uint8_t)(x + i), y, on);
    }
}

void gfx_vline(uint8_t x, uint8_t y, uint8_t height, bool on)
{
    if(x >= SSD1306_WIDTH || y >= SSD1306_HEIGHT){
        return;
    }

    for(uint8_t i = 0; i < height && i < SSD1306_HEIGHT - y; i++){
        gfx_pixel(x, (uint8_t)(y + i), on);
    }
}

void gfx_mark_all(void)
{
    for(uint8_t page = 0; page < SSD1306_PAGES; page++){
        gfx_dirty[page] = 0xFF;
    }
}

bool gfx_dirty_any(void)
{
    for(uint8_t page = 0; page < SSD1306_PAGES; page++){
        if(gfx_dirty[page] != 0){
            return true;
        }
    }
    return false;
}

bool gfx_dirty_all(void)
{
    for(uint8_t page = 0; page < SSD1306_PAGES; page++){
        if(gfx_dirty[page] != 0xFF){
            return false;
        }
    }
    return true;
}

uint8_t gfx_dirty_mask(uint8_t page)
{
    if(page >= SSD1306_PAGES){
        return 0;
    }
    return gfx_dirty[page];
}

uint16_t gfx_dirty_bytes(void)
{
    uint16_t total = 0;

    for(uint8_t page = 0; page < SSD1306_PAGES; page++){
        for(uint8_t seg = 0; seg < GFX_SEGMENTS; seg++){
            if(gfx_dirty[page] & (1 << seg)){
                total += GFX_SEGMENT_WIDTH;
            }
        }
    }
    return total;
}

bool gfx_next_window(uint8_t *page_first, uint8_t *page_last, uint8_t *col_first, uint8_t *col_last)
{
    if(gfx_dirty_all()){
        *page_first = 0;
        *page_last = SSD1306_PAGES - 1;
        *col_first = 0;
        *col_last = SSD1306_WIDTH - 1;
        return true;
    }

    for(uint8_t page = 0; page < SSD1306_PAGES; page++){
        uint8_t mask = gfx_dirty[page];

        if(mask == 0){
            continue;
        }

        uint8_t first = 0;

        while(!(mask & (1 << first))){
            first++;
        }

        uint8_t last = first;

        while(last + 1 < GFX_SEGMENTS && (mask & (1 << (last + 1)))){
            last++;
        }

        *page_first = page;
        *page_last = page;
        *col_first = (uint8_t)(first * GFX_SEGMENT_WIDTH);
        *col_last = (uint8_t)(last * GFX_SEGMENT_WIDTH + GFX_SEGMENT_WIDTH - 1);
        return true;
    }
    return false;
}

void gfx_clean_window(uint8_t page_first, uint8_t page_last, uint8_t col_first, uint8_t col_last)
{
    uint8_t first = col_first / GFX_SEGMENT_WIDTH;
    uint8_t last = col_last / GFX_SEGMENT_WIDTH;
    uint8_t mask = 0;

    for(uint8_t seg = first; seg <= last && seg < GFX_SEGMENTS; seg++){
        mask |= (uint8_t)(1 << seg);
    }

    for(uint8_t page = page_first; page <= page_last && page < SSD1306_PAGES; page++){
        gfx_dirty[page] &= (uint8_t)~mask;
    }
}
