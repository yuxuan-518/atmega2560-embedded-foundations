#ifndef DISPLAY_H
#define DISPLAY_H

#include <stdint.h>
#include <stdbool.h>

typedef enum {
    DISPLAY_OFF,
    DISPLAY_WAIT,
    DISPLAY_INIT,
    DISPLAY_FIRST,
    DISPLAY_SHOW,
    DISPLAY_RUN,
    DISPLAY_BACKOFF,
    DISPLAY_FAILED,
    DISPLAY_STOPPING,
    DISPLAY_BLANK
} display_state_t;

typedef enum {
    DISPLAY_FRAME_IDLE,
    DISPLAY_FRAME_RENDER,
    DISPLAY_FRAME_SEND
} display_frame_t;

typedef enum {
    DISPLAY_ON_OK,
    DISPLAY_ON_ALREADY,
    DISPLAY_ON_STOPPING,
    DISPLAY_ON_BAD_CONFIG
} display_on_t;

bool display_init(uint32_t now);
void display_update(uint32_t now);

display_on_t display_on(uint32_t now);
bool display_off(void);
bool display_set_page(uint8_t page);
bool display_refresh(void);

display_state_t display_get_state(void);
display_frame_t display_get_frame(void);
bool display_is_active(void);
uint8_t display_get_page(void);
uint32_t display_get_frames(void);
uint32_t display_get_full_frames(void);
uint32_t display_get_partial_frames(void);
uint32_t display_get_unchanged(void);
uint32_t display_get_errors(void);
uint8_t display_get_consecutive(void);
uint32_t display_get_merged(void);
uint32_t display_get_full_ms(void);
uint32_t display_get_full_took(void);

void display_request_report(uint32_t now);
bool display_report_pending(void);
void display_send_report(void);
bool display_event_pending(void);
void display_send_event(void);

#endif
