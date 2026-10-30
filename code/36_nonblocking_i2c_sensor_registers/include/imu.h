#ifndef IMU_H
#define IMU_H

#include <stdint.h>
#include <stdbool.h>
#include "i2c.h"

typedef enum {
    IMU_STOPPED,
    IMU_ID,
    IMU_RESET,
    IMU_RESET_WAIT,
    IMU_WAKE,
    IMU_CONFIG,
    IMU_ACCEL_CONFIG,
    IMU_VERIFY_CONFIG,
    IMU_VERIFY_POWER,
    IMU_READY,
    IMU_SAMPLE,
    IMU_BACKOFF,
    IMU_FAILED,
    IMU_ID_ERROR
} imu_state_t;

typedef enum {
    IMU_ERROR_NONE,
    IMU_ERROR_NO_DEVICE,
    IMU_ERROR_ID,
    IMU_ERROR_CONFIG,
    IMU_ERROR_BUS
} imu_error_t;

void imu_init(void);
void imu_update(uint32_t now);

bool imu_start(uint32_t now);
bool imu_read_once(uint32_t now);
bool imu_stop(uint32_t now);

imu_state_t imu_get_state(void);
bool imu_is_active(void);
bool imu_stopping(void);
imu_error_t imu_get_error(void);
i2c_result_t imu_get_last_result(void);
i2c_phase_t imu_get_last_phase(void);
uint8_t imu_get_id(void);
bool imu_has_data(void);
bool imu_is_fresh(uint32_t now);
void imu_get_accel(int16_t accel[3]);
uint32_t imu_get_samples(void);
uint32_t imu_get_errors(void);
uint8_t imu_get_consecutive(void);
uint32_t imu_get_data_ms(void);

void imu_request_report(uint32_t now);
bool imu_report_pending(void);
void imu_send_report(void);
bool imu_event_pending(void);
void imu_send_event(void);

#endif
