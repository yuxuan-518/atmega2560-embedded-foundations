#ifndef SERVO_CTRL_H
#define SERVO_CTRL_H

#include <stdint.h>
#include <stdbool.h>

typedef enum {
    SERVO_MODE_OFF,
    SERVO_MODE_FIXED,
    SERVO_MODE_POT
} servo_mode_t;

typedef enum {
    SERVO_STOP_NONE,
    SERVO_STOP_COMMAND,
    SERVO_STOP_ESTOP,
    SERVO_STOP_POT_STALE,
    SERVO_STOP_POT_INVALID
} servo_stop_t;

typedef enum {
    SERVO_CMD_OK,
    SERVO_CMD_BAD_VALUE,
    SERVO_CMD_ESTOP,
    SERVO_CMD_NO_DATA
} servo_cmd_t;

bool servo_ctrl_init(uint32_t now);
void servo_ctrl_update(uint32_t now);

servo_cmd_t servo_ctrl_angle(uint32_t angle, uint32_t now);
servo_cmd_t servo_ctrl_pot(uint8_t channel, uint32_t now);
servo_cmd_t servo_ctrl_speed(uint32_t speed);
bool servo_ctrl_off(void);

servo_mode_t servo_ctrl_get_mode(void);
uint8_t servo_ctrl_get_source(void);
int32_t servo_ctrl_get_target(void);
int32_t servo_ctrl_get_current(void);
uint32_t servo_ctrl_get_speed(void);
servo_stop_t servo_ctrl_get_stop(void);
uint16_t servo_ctrl_ticks(int32_t mdeg);
bool servo_ctrl_pot_fresh(uint8_t channel, uint32_t now);

void servo_ctrl_request_report(uint32_t now);
bool servo_ctrl_report_pending(void);
void servo_ctrl_send_report(void);
bool servo_ctrl_event_pending(void);
void servo_ctrl_send_event(void);

#endif
