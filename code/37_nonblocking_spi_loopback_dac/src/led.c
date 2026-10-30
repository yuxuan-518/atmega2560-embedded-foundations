#include "led.h"
#include "config.h"
#include "pwm.h"
#include "usart.h"
#include <avr/pgmspace.h>

static led_mode_t mode = LED_OFF;
static uint32_t period_ms = CONFIG_LED_PERIOD_DEFAULT_MS;
static uint32_t phase_ms = 0;
static uint32_t last_update = 0;
static bool pwm_ready = false;

uint8_t led_breathe_duty(uint32_t phase, uint32_t period)
{
    uint32_t half = period / 2;

    if(phase >= period){
        phase %= period;
    }

    if(phase < half){
        return (uint8_t)((phase * 255UL) / half);
    }

    return (uint8_t)(((period - phase) * 255UL) / (period - half));
}

bool led_init(void)
{
    pwm_ready = pwm4a_init(CONFIG_LED_PWM_HZ);
    mode = LED_OFF;
    return pwm_ready;
}

void led_update(uint32_t now)
{
    if(mode != LED_BREATHE){
        return;
    }

    uint32_t elapsed = now - last_update;

    if(elapsed < CONFIG_LED_UPDATE_MS){
        return;
    }

    last_update = now;

    uint32_t previous = phase_ms;
    uint32_t half = period_ms / 2;

    phase_ms = (phase_ms + elapsed % period_ms) % period_ms;

    uint8_t duty = led_breathe_duty(phase_ms, period_ms);

    if(elapsed < period_ms){
        bool wrapped = phase_ms < previous;

        if(wrapped && previous >= half){
            duty = 0;
        } else if(!wrapped && previous < half && phase_ms >= half){
            duty = 255;
        }
    }

    pwm4a_set_duty(duty);
}

void led_off(void)
{
    mode = LED_OFF;
    pwm4a_set_duty(0);
}

void led_set_duty(uint8_t duty)
{
    mode = LED_FIXED;
    pwm4a_set_duty(duty);
}

void led_breathe(uint32_t now)
{
    mode = LED_BREATHE;
    phase_ms = 0;
    last_update = now;
    pwm4a_set_duty(0);
}

void led_pot(bool sample_valid, uint8_t duty)
{
    mode = LED_POT;

    if(sample_valid){
        pwm4a_set_duty(duty);
    } else {
        pwm4a_set_duty(0);
    }
}

void led_pot_input(uint8_t duty)
{
    if(mode != LED_POT){
        return;
    }

    pwm4a_set_duty(duty);
}

void led_auto(bool on)
{
    mode = LED_AUTO;
    pwm4a_set_duty(on ? 255 : 0);
}

void led_auto_input(uint8_t duty)
{
    if(mode != LED_AUTO){
        return;
    }

    if(pwm4a_get_duty() != duty){
        pwm4a_set_duty(duty);
    }
}

bool led_set_period(uint32_t new_period)
{
    if(new_period < CONFIG_LED_PERIOD_MIN_MS || new_period > CONFIG_LED_PERIOD_MAX_MS){
        return false;
    }

    phase_ms = (phase_ms * new_period) / period_ms;
    period_ms = new_period;
    return true;
}

led_mode_t led_get_mode(void)
{
    return mode;
}

uint8_t led_get_duty(void)
{
    return pwm4a_get_duty();
}

uint32_t led_get_period(void)
{
    return period_ms;
}

void led_send_status(void)
{
    usart0_send_string_P(PSTR("LED: "));

    switch(mode){
        case LED_OFF:
            usart0_send_string_P(PSTR("OFF"));
            break;
        case LED_FIXED:
            usart0_send_string_P(PSTR("FIXED"));
            break;
        case LED_BREATHE:
            usart0_send_string_P(PSTR("BREATHE"));
            break;
        case LED_POT:
            usart0_send_string_P(PSTR("POT"));
            break;
        case LED_AUTO:
            usart0_send_string_P(PSTR("AUTO"));
            break;
    }

    usart0_send_string_P(PSTR(", DUTY: "));
    usart0_send_uint32(pwm4a_get_duty());
    usart0_send_string_P(PSTR("/255, PERIOD: "));
    usart0_send_uint32(period_ms);
    usart0_send_string_P(PSTR(" ms, PWM: "));
    usart0_send_uint32(pwm4a_get_hz());
    usart0_send_string_P(PSTR(" Hz, TOP: "));
    usart0_send_uint32(pwm4a_get_top());

    if(!pwm_ready){
        usart0_send_string_P(PSTR(", PWM INIT FAILED"));
    }

    usart0_send_string_P(PSTR(", Timer4 OC4A D6"));
    usart0_send_newline();
}
