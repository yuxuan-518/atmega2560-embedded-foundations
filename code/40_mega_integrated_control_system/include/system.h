#ifndef SYSTEM_H
#define SYSTEM_H

#include <stdint.h>
#include <stdbool.h>

typedef enum {
    SYS_IDLE,
    SYS_HOMING,
    SYS_MANUAL,
    SYS_RUNNING,
    SYS_STOPPING,
    SYS_FAULT
} sys_state_t;

#define SYS_WARN_DISPLAY 0x01
#define SYS_WARN_IMU 0x02
#define SYS_WARN_NH3 0x04
#define SYS_WARN_RANGE 0x08
#define SYS_WARN_ADC 0x10
#define SYS_WARN_SPI 0x20
#define SYS_WARN_COUNT 6

void system_init(uint32_t now);
void system_update(uint32_t now);

sys_state_t system_get_state(void);
sys_state_t system_get_recorded(void);
uint32_t system_get_state_ms(void);
uint32_t system_get_transitions(void);
uint8_t system_get_warnings(void);
uint32_t system_get_onsets(uint8_t index);

void system_send_state(sys_state_t state);
void system_send_warnings(uint8_t mask);
const char *system_state_name(sys_state_t state);
const char *system_warn_name(uint8_t index);

void system_request_report(uint32_t now);
bool system_report_pending(void);
void system_send_report(void);
bool system_event_pending(void);
void system_send_event(void);

#endif
