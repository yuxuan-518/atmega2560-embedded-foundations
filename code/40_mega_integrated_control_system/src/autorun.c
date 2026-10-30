#include "autorun.h"
#include "config.h"
#include "fault.h"
#include "homing.h"
#include "limit.h"
#include "motor.h"
#include "position.h"
#include "task.h"

static auto_phase_t phase = AUTO_IDLE;
static bool points_set = false;
static int32_t point_a = 0;
static int32_t point_b = 0;
static uint16_t cycles = CONFIG_SYSTEM_CYCLES_DEFAULT;
static run_result_t run;
static bool leg_active = false;
static bool leg_open = false;
static uint16_t leg = 0;

static int32_t autorun_leg_target(uint16_t index)
{
    return (index % 2 == 1) ? run.b : run.a;
}

static uint16_t autorun_cycles_from_legs(uint16_t legs)
{
    return legs == 0 ? 0 : (uint16_t)((legs - 1) / 2);
}

static void autorun_end(run_end_t reason)
{
    if(phase == AUTO_RUNNING){
        run.reason = reason;
        phase = AUTO_ENDING;
        leg_open = leg_active;
        leg_active = false;
    } else if(phase == AUTO_ENDING && reason == RUN_END_FAULT){
        run.reason = reason;
    }
}

static void autorun_finish(uint32_t now)
{
    run.final_pos = motor_get_position();
    run.end_ms = now;

    if(leg_open && run.final_pos == run.target && run.legs_done < run.legs_total){
        run.legs_done++;
    }
    leg_open = false;
    run.cycles_done = autorun_cycles_from_legs(run.legs_done);

    if(fault_active() && run.reason != RUN_END_DONE){
        run.reason = RUN_END_FAULT;
    } else if(run.legs_done >= run.legs_total && (run.reason == RUN_END_STOP || run.reason == RUN_END_ABORT)){
        run.reason = RUN_END_DONE;
    }
    run.fault = (run.reason == RUN_END_FAULT) ? fault_get() : FAULT_NONE;

    phase = AUTO_IDLE;
    leg_active = false;
    runlog_record(&run);
}

bool autorun_point_ok(int32_t point)
{
    const config_t *cfg = config_get();

    return point >= cfg->soft_min && point <= cfg->soft_max;
}

void autorun_init(void)
{
    phase = AUTO_IDLE;
    leg_open = false;
    points_set = false;
    cycles = CONFIG_SYSTEM_CYCLES_DEFAULT;
    leg_active = false;
    leg = 0;
}

auto_set_t autorun_set_points(int32_t a, int32_t b)
{
    if(phase != AUTO_IDLE){
        return AUTO_SET_BUSY;
    }

    if(a == b){
        return AUTO_SET_SAME;
    }

    if(!autorun_point_ok(a) || !autorun_point_ok(b)){
        return AUTO_SET_RANGE;
    }

    point_a = a;
    point_b = b;
    points_set = true;
    return AUTO_SET_OK;
}

auto_set_t autorun_set_cycles(int32_t value)
{
    if(phase != AUTO_IDLE){
        return AUTO_SET_BUSY;
    }

    if(value < 1 || value > CONFIG_SYSTEM_CYCLES_MAX){
        return AUTO_SET_RANGE;
    }

    cycles = (uint16_t)value;
    return AUTO_SET_OK;
}

auto_start_t autorun_start(uint32_t now)
{
    if(phase != AUTO_IDLE){
        return AUTO_START_ALREADY;
    }

    if(fault_active()){
        return AUTO_START_FAULT;
    }

    if(task_busy()){
        return AUTO_START_BUSY;
    }

    if(!homing_is_homed()){
        return AUTO_START_NOT_HOMED;
    }

    if(!points_set){
        return AUTO_START_NO_POINTS;
    }

    if(!autorun_point_ok(point_a) || !autorun_point_ok(point_b)){
        return AUTO_START_RANGE;
    }

    if(limit_any_active()){
        return AUTO_START_LIMIT;
    }

    run.number = runlog_next_number();
    run.a = point_a;
    run.b = point_b;
    run.cycles = cycles;
    run.cycles_done = 0;
    run.legs_done = 0;
    run.legs_total = (uint16_t)(2U * cycles + 1U);
    run.target = point_a;
    run.final_pos = motor_get_position();
    run.start_ms = now;
    run.end_ms = now;
    run.reason = RUN_END_NONE;
    run.fault = FAULT_NONE;
    leg = 0;

    if(!position_goto(point_a, false)){
        return AUTO_START_MOTION;
    }

    leg_active = true;
    phase = AUTO_RUNNING;
    return AUTO_START_OK;
}

static void autorun_step(void)
{
    if(leg_active){
        if(position_busy()){
            return;
        }

        leg_active = false;

        if(!position_last_ok()){
            autorun_end(RUN_END_MOVE_FAIL);
            return;
        }

        run.legs_done++;
        run.cycles_done = autorun_cycles_from_legs(run.legs_done);

        if(run.legs_done >= run.legs_total){
            autorun_end(RUN_END_DONE);
            return;
        }

        leg = run.legs_done;
    }

    run.target = autorun_leg_target(leg);

    if(!position_goto(run.target, false)){
        autorun_end(RUN_END_MOVE_FAIL);
        return;
    }

    leg_active = true;
}

void autorun_update(uint32_t now)
{
    if(phase == AUTO_IDLE){
        return;
    }

    if(phase == AUTO_RUNNING && fault_active()){
        autorun_end(RUN_END_FAULT);
    }

    if(phase == AUTO_RUNNING){
        autorun_step();
    }

    if(phase == AUTO_ENDING && !motor_is_moving() && !position_busy()){
        autorun_finish(now);
    }
}

void autorun_stop(void)
{
    autorun_end(RUN_END_STOP);
}

void autorun_abort(void)
{
    autorun_end(RUN_END_ABORT);
}

auto_phase_t autorun_get_phase(void)
{
    return phase;
}

bool autorun_busy(void)
{
    return phase != AUTO_IDLE;
}

bool autorun_points_set(void)
{
    return points_set;
}

int32_t autorun_get_a(void)
{
    return point_a;
}

int32_t autorun_get_b(void)
{
    return point_b;
}

uint16_t autorun_get_cycles(void)
{
    return phase == AUTO_IDLE ? cycles : run.cycles;
}

uint16_t autorun_get_leg(void)
{
    return leg;
}

uint16_t autorun_get_legs_done(void)
{
    return phase == AUTO_IDLE ? 0 : run.legs_done;
}

uint16_t autorun_get_legs_total(void)
{
    return phase == AUTO_IDLE ? (uint16_t)(2U * cycles + 1U) : run.legs_total;
}

uint16_t autorun_get_cycles_done(void)
{
    return phase == AUTO_IDLE ? 0 : run.cycles_done;
}

int32_t autorun_get_target(void)
{
    return run.target;
}

bool autorun_target_is_b(void)
{
    return leg % 2 == 1;
}

uint32_t autorun_get_elapsed(uint32_t now)
{
    return phase == AUTO_IDLE ? 0 : now - run.start_ms;
}
