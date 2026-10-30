#ifndef RUNLOG_H
#define RUNLOG_H

#include <stdint.h>
#include <stdbool.h>
#include "fault.h"

#define RUNLOG_QUEUE 4

typedef enum {
    RUN_END_NONE,
    RUN_END_DONE,
    RUN_END_STOP,
    RUN_END_FAULT,
    RUN_END_ABORT,
    RUN_END_MOVE_FAIL
} run_end_t;

typedef struct {
    uint32_t number;
    int32_t a;
    int32_t b;
    uint16_t cycles;
    uint16_t cycles_done;
    uint16_t legs_done;
    uint16_t legs_total;
    int32_t target;
    int32_t final_pos;
    uint32_t start_ms;
    uint32_t end_ms;
    run_end_t reason;
    fault_t fault;
} run_result_t;

void runlog_init(void);
uint32_t runlog_next_number(void);
void runlog_record(const run_result_t *result);

bool runlog_has_last(void);
const run_result_t *runlog_get_last(void);
uint32_t runlog_get_runs(void);
uint32_t runlog_get_count(run_end_t reason);
uint32_t runlog_get_cycles_total(void);

void runlog_send_reason(run_end_t reason, fault_t fault);
void runlog_send_result(const run_result_t *result);

bool runlog_summary_pending(void);
void runlog_send_summary(void);
uint32_t runlog_get_lost(void);
uint8_t runlog_get_queued(void);

#endif
