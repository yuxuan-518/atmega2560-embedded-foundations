#include "runlog.h"
#include "usart.h"
#include <avr/pgmspace.h>

#define RUNLOG_REASONS 6

static run_result_t last;
static bool has_last = false;
static run_result_t queue[RUNLOG_QUEUE];
static uint8_t queue_head = 0;
static uint8_t queue_count = 0;
static uint32_t lost = 0;
static uint32_t lost_unreported = 0;
static uint32_t runs = 0;
static uint32_t cycles_total = 0;
static uint32_t counts[RUNLOG_REASONS];

static void runlog_send_int32(int32_t number)
{
    if(number < 0){
        usart0_send_string_P(PSTR("-"));
        number = -number;
    }
    usart0_send_uint32((uint32_t)number);
}

void runlog_init(void)
{
    has_last = false;
    queue_head = 0;
    queue_count = 0;
    lost = 0;
    lost_unreported = 0;
    runs = 0;
    cycles_total = 0;

    for(uint8_t i = 0; i < RUNLOG_REASONS; i++){
        counts[i] = 0;
    }
}

uint32_t runlog_next_number(void)
{
    return runs + 1;
}

void runlog_record(const run_result_t *result)
{
    last = *result;
    has_last = true;

    if(queue_count < RUNLOG_QUEUE){
        queue[(uint8_t)((queue_head + queue_count) % RUNLOG_QUEUE)] = *result;
        queue_count++;
    } else {
        lost++;
        lost_unreported++;
    }

    runs++;
    cycles_total += result->cycles_done;

    if(result->reason < RUNLOG_REASONS){
        counts[result->reason]++;
    }
}

bool runlog_has_last(void)
{
    return has_last;
}

const run_result_t *runlog_get_last(void)
{
    return has_last ? &last : 0;
}

uint32_t runlog_get_runs(void)
{
    return runs;
}

uint32_t runlog_get_count(run_end_t reason)
{
    return reason < RUNLOG_REASONS ? counts[reason] : 0;
}

uint32_t runlog_get_cycles_total(void)
{
    return cycles_total;
}

void runlog_send_reason(run_end_t reason, fault_t fault)
{
    switch(reason){
        case RUN_END_NONE:
            usart0_send_string_P(PSTR("NONE"));
            break;
        case RUN_END_DONE:
            usart0_send_string_P(PSTR("DONE"));
            break;
        case RUN_END_STOP:
            usart0_send_string_P(PSTR("STOPPED"));
            break;
        case RUN_END_FAULT:
            usart0_send_string_P(PSTR("FAULT "));
            fault_send_name(fault);
            break;
        case RUN_END_ABORT:
            usart0_send_string_P(PSTR("ABORTED"));
            break;
        case RUN_END_MOVE_FAIL:
            usart0_send_string_P(PSTR("MOVE FAILED"));
            break;
    }
}

void runlog_send_result(const run_result_t *result)
{
    usart0_send_string_P(PSTR("#"));
    usart0_send_uint32(result->number);
    usart0_send_string_P(PSTR(" "));
    runlog_send_reason(result->reason, result->fault);
    usart0_send_string_P(PSTR(", CYCLES "));
    usart0_send_uint32(result->cycles_done);
    usart0_send_string_P(PSTR("/"));
    usart0_send_uint32(result->cycles);
    usart0_send_string_P(PSTR(", LEGS "));
    usart0_send_uint32(result->legs_done);
    usart0_send_string_P(PSTR("/"));
    usart0_send_uint32(result->legs_total);
    usart0_send_string_P(PSTR(", A "));
    runlog_send_int32(result->a);
    usart0_send_string_P(PSTR(" B "));
    runlog_send_int32(result->b);
    usart0_send_string_P(PSTR(", TARGET "));
    runlog_send_int32(result->target);
    usart0_send_string_P(PSTR(", FINAL "));
    runlog_send_int32(result->final_pos);
    usart0_send_string_P(PSTR(", "));
    usart0_send_uint32(result->end_ms - result->start_ms);
    usart0_send_string_P(PSTR(" ms"));
    usart0_send_newline();
}

bool runlog_summary_pending(void)
{
    return queue_count != 0 || lost_unreported != 0;
}

uint32_t runlog_get_lost(void)
{
    return lost;
}

uint8_t runlog_get_queued(void)
{
    return queue_count;
}

void runlog_send_summary(void)
{
    if(queue_count != 0){
        const run_result_t *result = &queue[queue_head];

        usart0_send_string_P(PSTR("["));
        usart0_send_uint32(result->end_ms);
        usart0_send_string_P(PSTR(" ms] AUTO END "));
        runlog_send_result(result);

        queue_head = (uint8_t)((queue_head + 1) % RUNLOG_QUEUE);
        queue_count--;
        return;
    }

    if(lost_unreported != 0){
        usart0_send_string_P(PSTR("AUTO END LOST "));
        usart0_send_uint32(lost_unreported);
        usart0_send_string_P(PSTR(" summaries (queue full), counts and last run in system status"));
        usart0_send_newline();
        lost_unreported = 0;
    }
}
