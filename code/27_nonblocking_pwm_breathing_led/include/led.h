#ifndef LED_H
#define LED_H

#include <stdint.h>
#include <stdbool.h>

typedef enum {
    LED_OFF,
    LED_FIXED,
    LED_BREATHE
} led_mode_t;

bool led_init(void);
void led_update(uint32_t now);

void led_off(void);
void led_set_duty(uint8_t duty);
void led_breathe(uint32_t now);
bool led_set_period(uint32_t period_ms);

led_mode_t led_get_mode(void);
uint8_t led_get_duty(void);
uint32_t led_get_period(void);
uint8_t led_breathe_duty(uint32_t phase, uint32_t period);
void led_send_status(void);

#endif
