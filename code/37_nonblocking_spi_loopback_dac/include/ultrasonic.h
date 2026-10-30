#ifndef ULTRASONIC_H
#define ULTRASONIC_H

#include <stdint.h>
#include <stdbool.h>

typedef struct {
    bool rise_seen;
    bool fall_seen;
    uint32_t rise_us;
    uint32_t fall_us;
} ultrasonic_echo_t;

void ultrasonic_init(void);
void ultrasonic_trig(bool high);
bool ultrasonic_echo_high(void);
void ultrasonic_arm(void);
void ultrasonic_disarm(void);
void ultrasonic_get_echo(ultrasonic_echo_t *echo);
uint32_t ultrasonic_get_stray(void);

#endif
