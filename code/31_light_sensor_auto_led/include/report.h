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
void report_set_log(bool enable);

#endif
