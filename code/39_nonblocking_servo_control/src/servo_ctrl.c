#include "servo_ctrl.h"
#include "config.h"
#include "servo.h"
#include "pot.h"
#include "analog.h"
#include "fault.h"
#include "estop.h"
#include "usart.h"

#include <avr/pgmspace.h>

#define SERVO_MDEG_MAX ((int32_t)CONFIG_SERVO_ANGLE_MAX * 1000L)

static bool ready = false;
static servo_mode_t mode = SERVO_MODE_OFF;
static uint8_t source = 0;
static int32_t target = (int32_t)CONFIG_SERVO_START_ANGLE * 1000L;
static int32_t current = (int32_t)CONFIG_SERVO_START_ANGLE * 1000L;
static uint32_t speed = CONFIG_SERVO_SPEED_DEFAULT;
static uint32_t step_ms = 0;
static servo_stop_t stop_reason = SERVO_STOP_NONE;
static uint32_t stops = 0;

typedef struct {
    servo_mode_t mode;
    uint8_t source;
    int32_t target;
    int32_t current;
    uint32_t speed;
    bool running;
    bool stopping;
    uint16_t output;
    uint16_t pending;
    uint32_t frames;
    bool pot_valid;
    bool pot_fresh;
    uint8_t pot_hold;
    uint32_t pot_age;
    servo_stop_t stop;
    uint32_t stops;
} servo_report_t;

static servo_report_t snap;
static uint8_t report_part = 0;

static bool event_pending = false;
static servo_stop_t event_reason = SERVO_STOP_NONE;
static uint8_t event_source = 0;
static int32_t event_current = 0;

uint16_t servo_ctrl_ticks(int32_t mdeg)
{
    if(mdeg < 0){
        mdeg = 0;
    }
    if(mdeg > SERVO_MDEG_MAX){
        mdeg = SERVO_MDEG_MAX;
    }

    uint32_t low = (uint32_t)CONFIG_SERVO_MIN_US * SERVO_TICKS_PER_US;
    uint32_t span = (uint32_t)(CONFIG_SERVO_MAX_US - CONFIG_SERVO_MIN_US) * SERVO_TICKS_PER_US;

    return (uint16_t)(low + ((uint32_t)mdeg * span + (uint32_t)SERVO_MDEG_MAX / 2) / (uint32_t)SERVO_MDEG_MAX);
}

static const adc_scan_result_t *servo_ctrl_pot_sample(uint8_t channel)
{
    if(channel >= CONFIG_POT_COUNT){
        return 0;
    }

    const adc_scan_result_t *result = analog_get(channel);

    if(result == 0 || !result->valid || result->samples == 0){
        return 0;
    }
    return result;
}

bool servo_ctrl_pot_fresh(uint8_t channel, uint32_t now)
{
    const adc_scan_result_t *result = servo_ctrl_pot_sample(channel);

    return result != 0 && now - result->sample_ms <= CONFIG_SERVO_POT_STALE_MS;
}

static bool servo_ctrl_pot_target(uint8_t channel, int32_t *out)
{
    pot_view_t view;

    if(!pot_is_valid(channel) || !pot_get_view(channel, &view) || !view.ready){
        return false;
    }

    *out = (int32_t)(((uint32_t)view.hold * (uint32_t)SERVO_MDEG_MAX + 127UL) / 255UL);
    return true;
}

static bool servo_ctrl_estop(void)
{
    return fault_get() == FAULT_ESTOP || estop_input_active();
}

static void servo_ctrl_stop(servo_stop_t reason)
{
    mode = SERVO_MODE_OFF;
    servo_stop();
    stop_reason = reason;
    stops++;

    if(reason != SERVO_STOP_COMMAND){
        event_reason = reason;
        event_source = source;
        event_current = current;
        event_pending = true;
    }
}

static void servo_ctrl_step(uint32_t now)
{
    if(now - step_ms < CONFIG_SERVO_STEP_MS){
        return;
    }

    step_ms = now;

    int32_t step = (int32_t)(speed * CONFIG_SERVO_STEP_MS);

    if(current < target){
        current = (target - current > step) ? current + step : target;
    } else if(current > target){
        current = (current - target > step) ? current - step : target;
    }

    servo_set(servo_ctrl_ticks(current));
}

bool servo_ctrl_init(uint32_t now)
{
    ready = servo_init(CONFIG_SERVO_PERIOD_US);
    mode = SERVO_MODE_OFF;
    target = (int32_t)CONFIG_SERVO_START_ANGLE * 1000L;
    current = target;
    speed = CONFIG_SERVO_SPEED_DEFAULT;
    step_ms = now;
    stop_reason = SERVO_STOP_NONE;
    stops = 0;
    report_part = 0;
    event_pending = false;
    return ready;
}

void servo_ctrl_update(uint32_t now)
{
    if(mode == SERVO_MODE_OFF){
        return;
    }

    if(servo_ctrl_estop()){
        servo_ctrl_stop(SERVO_STOP_ESTOP);
        return;
    }

    if(mode == SERVO_MODE_POT){
        int32_t value = 0;

        if(!servo_ctrl_pot_target(source, &value)){
            servo_ctrl_stop(SERVO_STOP_POT_INVALID);
            return;
        }

        if(!servo_ctrl_pot_fresh(source, now)){
            servo_ctrl_stop(SERVO_STOP_POT_STALE);
            return;
        }
        target = value;
    }

    servo_ctrl_step(now);
}

static void servo_ctrl_enable(uint32_t now)
{
    if(!servo_is_running()){
        step_ms = now;
    }
    servo_start(servo_ctrl_ticks(current));
    stop_reason = SERVO_STOP_NONE;
}

servo_cmd_t servo_ctrl_angle(uint32_t angle, uint32_t now)
{
    if(!ready || angle > CONFIG_SERVO_ANGLE_MAX){
        return SERVO_CMD_BAD_VALUE;
    }

    if(servo_ctrl_estop()){
        return SERVO_CMD_ESTOP;
    }

    target = (int32_t)angle * 1000L;
    mode = SERVO_MODE_FIXED;
    servo_ctrl_enable(now);
    return SERVO_CMD_OK;
}

servo_cmd_t servo_ctrl_pot(uint8_t channel, uint32_t now)
{
    int32_t value = 0;

    if(!ready || channel >= CONFIG_POT_COUNT){
        return SERVO_CMD_BAD_VALUE;
    }

    if(servo_ctrl_estop()){
        return SERVO_CMD_ESTOP;
    }

    if(!servo_ctrl_pot_fresh(channel, now) || !servo_ctrl_pot_target(channel, &value)){
        return SERVO_CMD_NO_DATA;
    }

    source = channel;
    target = value;
    mode = SERVO_MODE_POT;
    servo_ctrl_enable(now);
    return SERVO_CMD_OK;
}

servo_cmd_t servo_ctrl_speed(uint32_t new_speed)
{
    if(new_speed < CONFIG_SERVO_SPEED_MIN || new_speed > CONFIG_SERVO_SPEED_MAX){
        return SERVO_CMD_BAD_VALUE;
    }

    speed = new_speed;
    return SERVO_CMD_OK;
}

bool servo_ctrl_off(void)
{
    bool was_on = mode != SERVO_MODE_OFF || servo_is_running();

    servo_ctrl_stop(SERVO_STOP_COMMAND);
    return was_on;
}

servo_mode_t servo_ctrl_get_mode(void)
{
    return mode;
}

uint8_t servo_ctrl_get_source(void)
{
    return source;
}

int32_t servo_ctrl_get_target(void)
{
    return target;
}

int32_t servo_ctrl_get_current(void)
{
    return current;
}

uint32_t servo_ctrl_get_speed(void)
{
    return speed;
}

servo_stop_t servo_ctrl_get_stop(void)
{
    return stop_reason;
}

void servo_ctrl_request_report(uint32_t now)
{
    pot_view_t view;
    const adc_scan_result_t *sample = servo_ctrl_pot_sample(source);

    snap.mode = mode;
    snap.source = source;
    snap.target = target;
    snap.current = current;
    snap.speed = speed;
    snap.running = servo_is_running();
    snap.stopping = servo_is_stopping();
    snap.output = servo_get_output();
    snap.pending = servo_get_pending();
    snap.frames = servo_get_frames();
    snap.pot_valid = pot_is_valid(source) && pot_get_view(source, &view) && view.ready;
    snap.pot_hold = snap.pot_valid ? view.hold : 0;
    snap.pot_fresh = servo_ctrl_pot_fresh(source, now);
    snap.pot_age = sample != 0 ? now - sample->sample_ms : 0;
    snap.stop = stop_reason;
    snap.stops = stops;
    report_part = 1;
}

bool servo_ctrl_report_pending(void)
{
    return report_part != 0;
}

static void servo_send_mdeg(int32_t mdeg)
{
    uint32_t frac = (uint32_t)mdeg % 1000UL;

    usart0_send_uint32((uint32_t)mdeg / 1000UL);
    usart0_send_string_P(PSTR("."));

    if(frac < 100){
        usart0_send_string_P(PSTR("0"));
    }
    if(frac < 10){
        usart0_send_string_P(PSTR("0"));
    }
    usart0_send_uint32(frac);
}

static void servo_send_ticks(uint16_t ticks)
{
    usart0_send_uint32(ticks / SERVO_TICKS_PER_US);
    usart0_send_string_P((ticks % SERVO_TICKS_PER_US) ? PSTR(".5") : PSTR(".0"));
    usart0_send_string_P(PSTR(" us"));
}

static void servo_send_reason(servo_stop_t reason, uint8_t channel)
{
    if(reason == SERVO_STOP_NONE){
        usart0_send_string_P(PSTR("none"));
    } else if(reason == SERVO_STOP_COMMAND){
        usart0_send_string_P(PSTR("servo off command"));
    } else if(reason == SERVO_STOP_ESTOP){
        usart0_send_string_P(PSTR("ESTOP"));
    } else {
        usart0_send_string_P(PSTR("A"));
        usart0_send_uint32(channel);
        usart0_send_string_P(reason == SERVO_STOP_POT_STALE ? PSTR(" data stale (no new sample for 200 ms)") : PSTR(" data invalid"));
    }
}

static void servo_line_mode(void)
{
    usart0_send_string_P(PSTR("SERVO D8: "));

    if(snap.mode == SERVO_MODE_FIXED){
        usart0_send_string_P(PSTR("FIXED"));
    } else if(snap.mode == SERVO_MODE_POT){
        usart0_send_string_P(PSTR("POT A"));
        usart0_send_uint32(snap.source);
    } else {
        usart0_send_string_P(PSTR("OFF"));
    }

    if(snap.running){
        usart0_send_string_P(snap.stopping ? PSTR(", pulses ending after this pulse") : PSTR(", pulses ON"));
    } else {
        usart0_send_string_P(PSTR(", pulses stopped, pin low"));
    }
    usart0_send_string_P(PSTR(", frames "));
    usart0_send_uint32(snap.frames);
    usart0_send_newline();
}

static void servo_line_angle(void)
{
    usart0_send_string_P(PSTR("SERVO TARGET "));
    servo_send_mdeg(snap.target);
    usart0_send_string_P(PSTR(" deg, COMMAND "));
    servo_send_mdeg(snap.current);
    usart0_send_string_P(PSTR(" deg"));

    if(snap.mode != SERVO_MODE_OFF){
        usart0_send_string_P(snap.current == snap.target ? PSTR(" (at target)") : PSTR(" (moving)"));
    }
    usart0_send_string_P(PSTR(", no position feedback"));
    usart0_send_newline();
}

static void servo_line_pulse(void)
{
    usart0_send_string_P(PSTR("SERVO PULSE "));

    if(snap.running){
        servo_send_ticks(snap.output);
        usart0_send_string_P(PSTR(" (next "));
        servo_send_ticks(snap.pending);
        usart0_send_string_P(PSTR(")"));
    } else {
        usart0_send_string_P(PSTR("none"));
    }
    usart0_send_string_P(PSTR(", PERIOD "));
    usart0_send_uint32(CONFIG_SERVO_PERIOD_US);
    usart0_send_string_P(PSTR(" us, MAP 0-"));
    usart0_send_uint32(CONFIG_SERVO_ANGLE_MAX);
    usart0_send_string_P(PSTR(" deg = "));
    usart0_send_uint32(CONFIG_SERVO_MIN_US);
    usart0_send_string_P(PSTR("-"));
    usart0_send_uint32(CONFIG_SERVO_MAX_US);
    usart0_send_string_P(PSTR(" us"));
    usart0_send_newline();
}

static void servo_line_source(void)
{
    usart0_send_string_P(PSTR("SERVO SPEED "));
    usart0_send_uint32(snap.speed);
    usart0_send_string_P(PSTR(" deg/s, STEP 20 ms, A"));
    usart0_send_uint32(snap.source);

    if(snap.pot_valid){
        usart0_send_string_P(PSTR(" hold "));
        usart0_send_uint32(snap.pot_hold);
        usart0_send_string_P(PSTR("/255, age "));
        usart0_send_uint32(snap.pot_age);
        usart0_send_string_P(snap.pot_fresh ? PSTR(" ms FRESH") : PSTR(" ms STALE"));
    } else {
        usart0_send_string_P(PSTR(" no data"));
    }
    usart0_send_newline();
}

static void servo_line_stop(void)
{
    usart0_send_string_P(PSTR("SERVO LAST STOP "));
    servo_send_reason(snap.stop, snap.source);
    usart0_send_string_P(PSTR(", stops "));
    usart0_send_uint32(snap.stops);
    usart0_send_newline();
}

void servo_ctrl_send_report(void)
{
    if(report_part == 1){
        servo_line_mode();
    } else if(report_part == 2){
        servo_line_angle();
    } else if(report_part == 3){
        servo_line_pulse();
    } else if(report_part == 4){
        servo_line_source();
    } else {
        servo_line_stop();
        report_part = 0;
        return;
    }
    report_part++;
}

bool servo_ctrl_event_pending(void)
{
    return event_pending;
}

void servo_ctrl_send_event(void)
{
    event_pending = false;

    usart0_send_string_P(PSTR("SERVO OFF: "));
    servo_send_reason(event_reason, event_source);
    usart0_send_string_P(PSTR(", pulses stopped at "));
    servo_send_mdeg(event_current);
    usart0_send_string_P(PSTR(" deg, needs a new servo command"));
    usart0_send_newline();
}
