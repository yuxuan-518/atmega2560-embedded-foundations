#ifndef DISPLAY_VIEW_H
#define DISPLAY_VIEW_H

#include <stdint.h>
#include <stdbool.h>

#define DISPLAY_VIEW_ROWS 8

void display_view_take(uint32_t now, uint8_t page, uint32_t display_errors);
void display_view_render(uint8_t row);
void display_view_send_page_name(uint8_t page);
bool display_view_fault(void);
const char *display_view_row_text(void);

#endif
