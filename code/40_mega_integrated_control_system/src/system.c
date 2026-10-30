#include "system.h"
#include "config.h"
#include "autorun.h"
#include "runlog.h"
#include "task.h"
#include "fault.h"
#include "motor.h"
#include "homing.h"
#include "position.h"
#include "nh3.h"
#include "range.h"
#include "imu.h"
#include "pot.h"
#include "light.h"
#include "loopback.h"
#include "display.h"
#include "buzzer.h"
#include "report.h"
#include "usart.h"
#include "command/command_motor.h"
#include <avr/pgmspace.h>

typedef struct {
    uint32_t now;
    sys_state_t state;
    uint32_t state_ms;
    uint32_t transitions;
    bool homed;
    fault_t fault;
    bool points_set;
    int32_t a;
    int32_t b;
    uint16_t cycles;
    auto_phase_t phase;
    uint16_t leg;
    uint16_t legs_done;
    uint16_t legs_total;
    uint16_t cycles_done;
    bool to_b;
    uint32_t elapsed;
    motor_state_t motor;
    int32_t pos;
    bool positioning;
    int32_t pos_target;
    uint32_t hz;
    nh3_state_t nh3;
    uint32_t nh3_errors;
    uint32_t warm_left;
    uint32_t ppb;
    bool range_on;
    bool range_fresh;
    uint16_t range_mm;
    range_result_t range_result;
    imu_state_t imu;
    bool imu_fresh;
    bool adc_ok;
    display_state_t display;
    loop_state_t loop;
    uint8_t warnings;
    uint32_t onsets[SYS_WARN_COUNT];
    uint32_t dropped;
    uint32_t runs;
    uint32_t done;
    uint32_t stopped;
    uint32_t faulted;
    uint32_t other;
    uint32_t cycles_total;
    uint32_t summaries_lost;
    uint8_t summaries_queued;
    bool has_last;
    run_result_t last;
} system_snap_t;

static sys_state_t state = SYS_IDLE;
static uint32_t state_ms = 0;
static uint32_t transitions = 0;
static sys_state_t logged_state = SYS_IDLE;
static uint32_t logged_transitions = 0;
static uint8_t warnings = 0;
static uint8_t logged_warnings = 0;
static uint32_t warn_ms = 0;
static uint32_t onsets[SYS_WARN_COUNT];
static uint32_t boot_ms = 0;
static bool adc_grace_done = false;
static bool link_grace_done = false;
static uint32_t seen_runs = 0;
static uint8_t report_part = 0;
static system_snap_t snap;

static const char state_idle[] PROGMEM = "IDLE";
static const char state_homing[] PROGMEM = "HOMING";
static const char state_manual[] PROGMEM = "MANUAL";
static const char state_running[] PROGMEM = "RUNNING";
static const char state_stopping[] PROGMEM = "STOPPING";
static const char state_fault[] PROGMEM = "FAULT";

static const char *const state_names[] PROGMEM = {
    state_idle,
    state_homing,
    state_manual,
    state_running,
    state_stopping,
    state_fault
};

static const char warn_display[] PROGMEM = "OLED";
static const char warn_imu[] PROGMEM = "IMU";
static const char warn_nh3[] PROGMEM = "NH3";
static const char warn_range[] PROGMEM = "DIST";
static const char warn_adc[] PROGMEM = "ADC";
static const char warn_spi[] PROGMEM = "SPI";

static const char *const warn_names[] PROGMEM = {
    warn_display,
    warn_imu,
    warn_nh3,
    warn_range,
    warn_adc,
    warn_spi
};

static void system_send_int32(int32_t number)
{
    if(number < 0){
        usart0_send_string_P(PSTR("-"));
        number = -number;
    }
    usart0_send_uint32((uint32_t)number);
}

const char *system_state_name(sys_state_t value)
{
    if(value > SYS_FAULT){
        value = SYS_FAULT;
    }
    return (const char *)pgm_read_word(&state_names[value]);
}

const char *system_warn_name(uint8_t index)
{
    if(index >= SYS_WARN_COUNT){
        index = 0;
    }
    return (const char *)pgm_read_word(&warn_names[index]);
}

void system_send_state(sys_state_t value)
{
    usart0_send_string_P(system_state_name(value));
}

void system_send_warnings(uint8_t mask)
{
    bool first = true;

    for(uint8_t i = 0; i < SYS_WARN_COUNT; i++){
        if(mask & (1 << i)){
            if(!first){
                usart0_send_string_P(PSTR(" "));
            }
            usart0_send_string_P(system_warn_name(i));
            first = false;
        }
    }

    if(first){
        usart0_send_string_P(PSTR("none"));
    }
}

sys_state_t system_get_state(void)
{
    auto_phase_t phase = autorun_get_phase();

    if(fault_active()){
        return SYS_FAULT;
    }

    if(phase == AUTO_ENDING || motor_get_state() == MOTOR_STOPPING){
        return SYS_STOPPING;
    }

    if(phase == AUTO_RUNNING){
        return SYS_RUNNING;
    }

    if(homing_busy()){
        return SYS_HOMING;
    }

    if(task_busy()){
        return SYS_MANUAL;
    }

    return SYS_IDLE;
}

static bool system_imu_warning(uint32_t now)
{
    imu_state_t imu = imu_get_state();

    if(imu == IMU_FAILED || imu == IMU_ID_ERROR){
        return true;
    }

    if(imu == IMU_STOPPED){
        return false;
    }

    return imu_get_consecutive() != 0 || (imu_has_data() && !imu_is_fresh(now));
}

static uint8_t system_eval_warnings(uint32_t now)
{
    uint8_t mask = 0;
    display_state_t display = display_get_state();
    nh3_state_t nh3 = nh3_get_state();
    loop_state_t loop = loopback_get_state();
    uint32_t age = now - boot_ms;

    if(age >= CONFIG_SYSTEM_ADC_GRACE_MS){
        adc_grace_done = true;
    }

    if(age >= CONFIG_SYSTEM_LINK_GRACE_MS){
        link_grace_done = true;
    }

    if(display == DISPLAY_FAILED || (display != DISPLAY_OFF && display_get_consecutive() != 0)){
        mask |= SYS_WARN_DISPLAY;
    }

    if(system_imu_warning(now)){
        mask |= SYS_WARN_IMU;
    }

    if(nh3 == NH3_STALE || (nh3 == NH3_NO_DATA && link_grace_done)){
        mask |= SYS_WARN_NH3;
    }

    if(range_is_running() && range_get_measurements() != 0 && !range_is_fresh(now)){
        mask |= SYS_WARN_RANGE;
    }

    if(adc_grace_done && (!pot_is_valid(0) || !pot_is_valid(1) || light_get_data() != LIGHT_OK)){
        mask |= SYS_WARN_ADC;
    }

    if(loop == LOOP_FAIL || loop == LOOP_ERROR){
        mask |= SYS_WARN_SPI;
    }

    return mask;
}

static void system_prompt(uint32_t now)
{
    uint32_t runs = runlog_get_runs();

    if(runs == seen_runs){
        return;
    }

    seen_runs = runs;

    const run_result_t *last = runlog_get_last();

    if(last == 0 || buzzer_get_mode() != BUZZER_IDLE){
        return;
    }

    if(last->reason == RUN_END_DONE){
        buzzer_tone(CONFIG_SYSTEM_BEEP_DONE_HZ, CONFIG_SYSTEM_BEEP_DONE_MS, now);
    } else if(last->reason == RUN_END_FAULT){
        buzzer_tone(CONFIG_SYSTEM_BEEP_FAULT_HZ, CONFIG_SYSTEM_BEEP_FAULT_MS, now);
    }
}

void system_init(uint32_t now)
{
    autorun_init();
    runlog_init();
    state = SYS_IDLE;
    state_ms = now;
    transitions = 0;
    logged_state = SYS_IDLE;
    logged_transitions = 0;
    warnings = 0;
    logged_warnings = 0;
    warn_ms = now;
    boot_ms = now;
    adc_grace_done = false;
    link_grace_done = false;
    seen_runs = 0;
    report_part = 0;

    for(uint8_t i = 0; i < SYS_WARN_COUNT; i++){
        onsets[i] = 0;
    }
}

void system_update(uint32_t now)
{
    sys_state_t current = system_get_state();

    if(current != state){
        state = current;
        state_ms = now;
        transitions++;
    }

    uint8_t mask = system_eval_warnings(now);

    if(mask != warnings){
        uint8_t added = (uint8_t)(mask & (uint8_t)~warnings);

        for(uint8_t i = 0; i < SYS_WARN_COUNT; i++){
            if(added & (1 << i)){
                onsets[i]++;
            }
        }

        warnings = mask;
        warn_ms = now;
    }

    system_prompt(now);
}

sys_state_t system_get_recorded(void)
{
    return state;
}

uint32_t system_get_state_ms(void)
{
    return state_ms;
}

uint32_t system_get_transitions(void)
{
    return transitions;
}

uint8_t system_get_warnings(void)
{
    return warnings;
}

uint32_t system_get_onsets(uint8_t index)
{
    return index < SYS_WARN_COUNT ? onsets[index] : 0;
}

void system_request_report(uint32_t now)
{
    const run_result_t *last = runlog_get_last();

    snap.now = now;
    snap.state = system_get_state();
    snap.state_ms = state_ms;
    snap.transitions = transitions;
    snap.homed = homing_is_homed();
    snap.fault = fault_get();
    snap.points_set = autorun_points_set();
    snap.a = autorun_get_a();
    snap.b = autorun_get_b();
    snap.cycles = autorun_get_cycles();
    snap.phase = autorun_get_phase();
    snap.leg = autorun_get_leg();
    snap.legs_done = autorun_get_legs_done();
    snap.legs_total = autorun_get_legs_total();
    snap.cycles_done = autorun_get_cycles_done();
    snap.to_b = autorun_target_is_b();
    snap.elapsed = autorun_get_elapsed(now);
    snap.motor = motor_get_state();
    snap.pos = motor_get_position();
    snap.positioning = position_busy();
    snap.pos_target = position_get_target();
    snap.hz = motor_get_current_hz();
    snap.nh3 = nh3_get_state();
    snap.nh3_errors = nh3_get_errors();
    snap.warm_left = nh3_get_warmup_left(now);
    snap.ppb = 0;
    nh3_get_ppb(&snap.ppb);
    snap.range_on = range_is_running();
    snap.range_fresh = range_is_fresh(now);
    snap.range_mm = range_get_filtered();
    snap.range_result = range_get_result();
    snap.imu = imu_get_state();
    snap.imu_fresh = imu_is_fresh(now);
    snap.adc_ok = pot_is_valid(0) && pot_is_valid(1) && light_get_data() == LIGHT_OK;
    snap.display = display_get_state();
    snap.loop = loopback_get_state();
    snap.warnings = warnings;

    for(uint8_t i = 0; i < SYS_WARN_COUNT; i++){
        snap.onsets[i] = onsets[i];
    }

    snap.dropped = report_get_dropped();
    snap.runs = runlog_get_runs();
    snap.done = runlog_get_count(RUN_END_DONE);
    snap.stopped = runlog_get_count(RUN_END_STOP);
    snap.faulted = runlog_get_count(RUN_END_FAULT);
    snap.other = snap.runs - snap.done - snap.stopped - snap.faulted;
    snap.cycles_total = runlog_get_cycles_total();
    snap.summaries_lost = runlog_get_lost();
    snap.summaries_queued = runlog_get_queued();
    snap.has_last = last != 0;

    if(last != 0){
        snap.last = *last;
    }
    report_part = 1;
}

bool system_report_pending(void)
{
    return report_part != 0;
}

static void system_line_state(void)
{
    usart0_send_string_P(PSTR("SYSTEM "));
    system_send_state(snap.state);
    usart0_send_string_P(PSTR(" for "));
    usart0_send_uint32(snap.now - snap.state_ms);
    usart0_send_string_P(PSTR(" ms, HOMED "));
    usart0_send_uint32(snap.homed);
    usart0_send_string_P(PSTR(", FAULT "));
    fault_send_name(snap.fault);
    usart0_send_string_P(PSTR(", CHANGES "));
    usart0_send_uint32(snap.transitions);
    usart0_send_newline();
}

static void system_line_plan(void)
{
    usart0_send_string_P(PSTR("PLAN "));

    if(snap.points_set){
        usart0_send_string_P(PSTR("A "));
        system_send_int32(snap.a);
        usart0_send_string_P(PSTR(" B "));
        system_send_int32(snap.b);
    } else {
        usart0_send_string_P(PSTR("points not set"));
    }
    usart0_send_string_P(PSTR(", CYCLES "));
    usart0_send_uint32(snap.cycles);
    usart0_send_string_P(PSTR(": first go to A, 1 cycle = A->B->A, ends at A"));
    usart0_send_newline();
}

static void system_line_auto(void)
{
    usart0_send_string_P(PSTR("AUTO "));

    if(snap.phase == AUTO_IDLE){
        usart0_send_string_P(PSTR("IDLE"));
    } else {
        usart0_send_string_P(snap.phase == AUTO_RUNNING ? PSTR("RUNNING") : PSTR("ENDING"));
        usart0_send_string_P(PSTR(", LEG "));
        usart0_send_uint32(snap.leg + 1UL);
        usart0_send_string_P(PSTR("/"));
        usart0_send_uint32(snap.legs_total);

        if(snap.phase == AUTO_RUNNING){
            if(snap.leg == 0){
                usart0_send_string_P(PSTR(" to A (approach)"));
            } else {
                usart0_send_string_P(snap.to_b ? PSTR(" to B") : PSTR(" to A"));
            }
        }
        usart0_send_string_P(PSTR(", CYCLES DONE "));
        usart0_send_uint32(snap.cycles_done);
        usart0_send_string_P(PSTR("/"));
        usart0_send_uint32(snap.cycles);
        usart0_send_string_P(PSTR(", "));
        usart0_send_uint32(snap.elapsed);
        usart0_send_string_P(PSTR(" ms"));
    }
    usart0_send_newline();
}

static void system_line_motion(void)
{
    usart0_send_string_P(PSTR("MOTION "));
    command_motor_send_state(snap.motor);
    usart0_send_string_P(PSTR(", POS "));
    system_send_int32(snap.pos);
    usart0_send_string_P(PSTR(", TARGET "));

    if(snap.positioning){
        system_send_int32(snap.pos_target);
    } else {
        usart0_send_string_P(PSTR("-"));
    }
    usart0_send_string_P(PSTR(", HZ "));
    usart0_send_uint32(snap.hz);
    usart0_send_newline();
}

static void system_send_display_state(display_state_t value)
{
    if(value == DISPLAY_OFF){
        usart0_send_string_P(PSTR("OFF"));
    } else if(value == DISPLAY_RUN){
        usart0_send_string_P(PSTR("RUN"));
    } else if(value == DISPLAY_BACKOFF){
        usart0_send_string_P(PSTR("RETRY"));
    } else if(value == DISPLAY_FAILED){
        usart0_send_string_P(PSTR("FAILED"));
    } else {
        usart0_send_string_P(PSTR("BUSY"));
    }
}

static void system_send_imu_state(void)
{
    if(snap.imu == IMU_STOPPED){
        usart0_send_string_P(PSTR("OFF"));
    } else if(snap.imu == IMU_FAILED){
        usart0_send_string_P(PSTR("FAILED"));
    } else if(snap.imu == IMU_ID_ERROR){
        usart0_send_string_P(PSTR("ID ERROR"));
    } else if(snap.imu == IMU_BACKOFF){
        usart0_send_string_P(PSTR("RETRY"));
    } else if(snap.imu_fresh){
        usart0_send_string_P(PSTR("OK"));
    } else {
        usart0_send_string_P(PSTR("WAIT"));
    }
}

static void system_line_sensors(void)
{
    static const char loops[][5] PROGMEM = {
        "IDLE", "RUN", "PASS", "FAIL", "ERR", "STOP"
    };

    usart0_send_string_P(PSTR("SENSORS NH3 "));
    nh3_send_state(snap.nh3);

    if(snap.nh3 == NH3_VALID){
        usart0_send_string_P(PSTR(" "));
        usart0_send_uint32(snap.ppb);
        usart0_send_string_P(PSTR(" ppb"));
    } else if(snap.nh3 == NH3_WARMUP && snap.warm_left != 0){
        usart0_send_string_P(PSTR(" "));
        usart0_send_uint32((snap.warm_left + 999UL) / 1000UL);
        usart0_send_string_P(PSTR(" s"));
    }
    usart0_send_string_P(PSTR(" ERR "));
    usart0_send_uint32(snap.nh3_errors);
    usart0_send_string_P(PSTR(", DIST "));

    if(!snap.range_on){
        usart0_send_string_P(PSTR("OFF"));
    } else if(snap.range_fresh){
        usart0_send_uint32(snap.range_mm);
        usart0_send_string_P(PSTR(" mm"));
    } else if(snap.range_result == RANGE_NONE){
        usart0_send_string_P(PSTR("WAIT"));
    } else {
        usart0_send_string_P(snap.range_result == RANGE_OK ? PSTR("STALE") : PSTR("NO ECHO"));
    }
    usart0_send_string_P(PSTR(", IMU "));
    system_send_imu_state();
    usart0_send_string_P(snap.adc_ok ? PSTR(", ADC OK, OLED ") : PSTR(", ADC BAD, OLED "));
    system_send_display_state(snap.display);
    usart0_send_string_P(PSTR(", SPI "));

    if(snap.loop <= LOOP_STOPPED){
        usart0_send_string_P(loops[snap.loop]);
    }
    usart0_send_newline();
}

static void system_line_warnings(void)
{
    usart0_send_string_P(PSTR("WARN "));
    system_send_warnings(snap.warnings);
    usart0_send_string_P(PSTR(" (no motion stop), ONSETS"));

    for(uint8_t i = 0; i < SYS_WARN_COUNT; i++){
        usart0_send_string_P(PSTR(" "));
        usart0_send_string_P(system_warn_name(i));
        usart0_send_string_P(PSTR(" "));
        usart0_send_uint32(snap.onsets[i]);
    }
    usart0_send_string_P(PSTR(", TX DROPPED "));
    usart0_send_uint32(snap.dropped);
    usart0_send_newline();
}

static void system_line_runs(void)
{
    usart0_send_string_P(PSTR("RUNS "));
    usart0_send_uint32(snap.runs);
    usart0_send_string_P(PSTR(": DONE "));
    usart0_send_uint32(snap.done);
    usart0_send_string_P(PSTR(", STOPPED "));
    usart0_send_uint32(snap.stopped);
    usart0_send_string_P(PSTR(", FAULT "));
    usart0_send_uint32(snap.faulted);
    usart0_send_string_P(PSTR(", OTHER "));
    usart0_send_uint32(snap.other);
    usart0_send_string_P(PSTR(", CYCLES "));
    usart0_send_uint32(snap.cycles_total);
    usart0_send_string_P(PSTR(", SUMMARY QUEUED "));
    usart0_send_uint32(snap.summaries_queued);
    usart0_send_string_P(PSTR(" LOST "));
    usart0_send_uint32(snap.summaries_lost);
    usart0_send_newline();
}

static void system_line_last(void)
{
    usart0_send_string_P(PSTR("LAST RUN "));

    if(snap.has_last){
        runlog_send_result(&snap.last);
    } else {
        usart0_send_string_P(PSTR("none"));
        usart0_send_newline();
    }
}

void system_send_report(void)
{
    if(report_part == 1){
        system_line_state();
    } else if(report_part == 2){
        system_line_plan();
    } else if(report_part == 3){
        system_line_auto();
    } else if(report_part == 4){
        system_line_motion();
    } else if(report_part == 5){
        system_line_sensors();
    } else if(report_part == 6){
        system_line_warnings();
    } else if(report_part == 7){
        system_line_runs();
    } else {
        system_line_last();
        report_part = 0;
        return;
    }
    report_part++;
}

bool system_event_pending(void)
{
    return state != logged_state || warnings != logged_warnings;
}

void system_send_event(void)
{
    usart0_send_string_P(PSTR("["));

    if(state != logged_state){
        usart0_send_uint32(state_ms);
        usart0_send_string_P(PSTR(" ms] SYSTEM "));
        system_send_state(logged_state);
        usart0_send_string_P(PSTR(" -> "));
        system_send_state(state);

        if(transitions - logged_transitions > 1){
            usart0_send_string_P(PSTR(" ("));
            usart0_send_uint32(transitions - logged_transitions);
            usart0_send_string_P(PSTR(" changes)"));
        }
        logged_state = state;
        logged_transitions = transitions;
    } else {
        usart0_send_uint32(warn_ms);
        usart0_send_string_P(PSTR(" ms] SYSTEM WARN "));
        system_send_warnings(warnings);
        logged_warnings = warnings;
    }
    usart0_send_newline();
}
