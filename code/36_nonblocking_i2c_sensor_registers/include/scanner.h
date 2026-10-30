#ifndef SCANNER_H
#define SCANNER_H

#include <stdint.h>
#include <stdbool.h>

typedef enum {
    SCANNER_IDLE,
    SCANNER_RUNNING,
    SCANNER_DONE,
    SCANNER_CANCELLED,
    SCANNER_FAULT
} scanner_state_t;

typedef enum {
    SCANNER_FAULT_NONE,
    SCANNER_FAULT_SDA_LOW,
    SCANNER_FAULT_SCL_LOW,
    SCANNER_FAULT_BOTH_LOW,
    SCANNER_FAULT_ARB_LOST,
    SCANNER_FAULT_BUS_ERROR,
    SCANNER_FAULT_TIMEOUT,
    SCANNER_FAULT_BAD_STATUS
} scanner_fault_t;

void scanner_init(void);
void scanner_update(uint32_t now);

bool scanner_start(uint32_t now);
bool scanner_stop(uint32_t now);

scanner_state_t scanner_get_state(void);
scanner_fault_t scanner_get_fault(void);
bool scanner_cancelling(void);
uint8_t scanner_get_address(void);
bool scanner_found(uint8_t address);
uint8_t scanner_found_count(void);
uint32_t scanner_get_count(uint8_t result);
uint32_t scanner_get_retries(void);
uint32_t scanner_get_duration(uint32_t now);

void scanner_request_report(uint32_t now);
bool scanner_head_pending(void);
void scanner_send_head(void);
bool scanner_list_pending(void);
void scanner_send_list_line(void);

#endif
