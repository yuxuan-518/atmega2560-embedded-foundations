#ifndef REPORT_H
#define REPORT_H

#include <stdint.h>
#include <stdbool.h>

#define REPORT_TX_RESERVE 96
#define REPORT_STATUS_BYTES 200
#define REPORT_CONFIG_BYTES 210
#define REPORT_NH3_BYTES 220
#define REPORT_NH3_BRIEF_BYTES 60
#define REPORT_LED_BYTES 110
#define REPORT_ADC_BYTES 320
#define REPORT_LOG_BYTES 96
#define REPORT_POT_BYTES 300
#define REPORT_LIGHT_BYTES 200
#define REPORT_BUZZER_BYTES 150
#define REPORT_COUNTER_BYTES 300
#define REPORT_COUNTER_LINE_BYTES 72
#define REPORT_RANGE_BYTES 400
#define REPORT_RANGE_EVENT_BYTES 56
#define REPORT_I2C_HEAD_BYTES 260
#define REPORT_I2C_LINE_BYTES 64
#define REPORT_IMU_BYTES 330
#define REPORT_IMU_EVENT_BYTES 80
#define REPORT_SPI_LINE_BYTES 160
#define REPORT_SPI_EVENT_BYTES 150
#define REPORT_DAC_LINE_BYTES 110
#define REPORT_DAC_EVENT_BYTES 64
#define REPORT_LINE_BYTES 40
#define REPORT_HEARTBEAT_MS 1000
#define REPORT_LOG_MS 100

void report_update(uint32_t now);
void report_request_status(void);
void report_request_config(void);
void report_request_help(void);
void report_request_nh3(void);
void report_request_led(void);
void report_request_adc(void);
void report_request_pot(void);
void report_request_light(void);
void report_request_buzzer(void);
void report_request_counter(void);
void report_request_range(void);
void report_set_log(bool enable);

#endif
