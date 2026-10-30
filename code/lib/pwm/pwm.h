#ifndef PWM_H
#define PWM_H

#include <stdint.h>
#include <stdbool.h>

#define PWM4_PRESCALER 8UL
#define PWM4_MIN_HZ 31UL
#define PWM4_MAX_HZ 7812UL

bool pwm4a_init(uint32_t Hz);
void pwm4a_set_duty(uint8_t duty);
uint8_t pwm4a_get_duty(void);
uint32_t pwm4a_get_hz(void);
uint16_t pwm4a_get_top(void);

#endif
