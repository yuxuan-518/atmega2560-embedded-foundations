#include "servo.h"
#include <avr/io.h>
#include <avr/interrupt.h>
#include <util/atomic.h>

#define SERVO_BIT (1 << PORTH5)
#define SERVO_FAST (1 << CS01)
#define SERVO_SLOW ((1 << CS01) | (1 << CS00))
#define SERVO_SLOW_UNITS 8
#define SERVO_SEGMENT_MAX 250
#define SERVO_SEGMENT_MIN 64

typedef struct {
    uint16_t width;
    uint8_t high_seg;
    uint8_t high_count;
    uint8_t high_extra;
    uint8_t low_seg;
    uint8_t low_count;
    uint8_t low_extra;
} servo_plan_t;

static volatile bool servo_running = false;
static volatile bool servo_stop_request = false;
static volatile bool servo_high = false;
static volatile uint16_t servo_output = 0;
static volatile uint32_t servo_frames = 0;
static servo_plan_t servo_pending;
static servo_plan_t servo_frame;
static uint8_t servo_seg = 0;
static uint8_t servo_left = 0;
static uint8_t servo_extra = 0;
static uint32_t servo_period_units = 40000;
static uint32_t servo_period_us = 20000;

static void servo_split(uint16_t ticks, uint8_t *seg, uint8_t *count, uint8_t *extra)
{
    uint16_t n = (uint16_t)((ticks + SERVO_SEGMENT_MAX - 1) / SERVO_SEGMENT_MAX);

    *count = (uint8_t)n;
    *seg = (uint8_t)(ticks / n);
    *extra = (uint8_t)(ticks % n);
}

static void servo_plan(uint16_t width, servo_plan_t *plan)
{
    plan->width = width;
    servo_split((uint16_t)(width - SERVO_COMP_TICKS), &plan->high_seg, &plan->high_count, &plan->high_extra);
    servo_split((uint16_t)((servo_period_units - width + SERVO_SLOW_UNITS / 2) / SERVO_SLOW_UNITS), &plan->low_seg, &plan->low_count, &plan->low_extra);
}

static void servo_halt(void)
{
    TIMSK0 &= ~(1 << OCIE0A);
    TCCR0B = 0;
    TIFR0 = (1 << OCF0A);

    if(PORTH & SERVO_BIT){
        PINH = SERVO_BIT;
    }

    servo_high = false;
    servo_running = false;
    servo_stop_request = false;
}

ISR(TIMER0_COMPA_vect)
{
    if(servo_left == 0){
        if(servo_high){
            PINH = SERVO_BIT;
            servo_high = false;

            if(servo_stop_request){
                TIMSK0 &= ~(1 << OCIE0A);
                TCCR0B = 0;
                servo_running = false;
                servo_stop_request = false;
                return;
            }

            TCNT0 = 0;
            TCCR0B = SERVO_SLOW;
            servo_seg = servo_frame.low_seg;
            servo_left = servo_frame.low_count;
            servo_extra = servo_frame.low_extra;
        } else {
            if(!(PORTH & SERVO_BIT)){
                PINH = SERVO_BIT;
            }
            TCNT0 = 0;
            TCCR0B = SERVO_FAST;
            servo_frame = servo_pending;
            servo_seg = servo_frame.high_seg;
            servo_left = servo_frame.high_count;
            servo_extra = servo_frame.high_extra;
            servo_output = servo_frame.width;
            servo_frames++;
            servo_high = true;
        }
    }

    uint8_t seg = servo_seg;

    if(servo_left <= servo_extra){
        seg++;
    }

    servo_left--;
    OCR0A = (uint8_t)(seg - 1);
}

bool servo_init(uint32_t period_us)
{
    if(period_us < SERVO_PERIOD_MIN_US || period_us > SERVO_PERIOD_MAX_US){
        return false;
    }

    ATOMIC_BLOCK(ATOMIC_RESTORESTATE){
        servo_halt();
        TCCR0A = (1 << WGM01);
        TCNT0 = 0;
        PORTH &= ~SERVO_BIT;
        DDRH |= SERVO_BIT;
        servo_period_us = period_us;
        servo_period_units = period_us * SERVO_TICKS_PER_US;
        servo_frames = 0;
        servo_output = 0;
        servo_left = 0;
    }
    return true;
}

static bool servo_valid(uint16_t ticks)
{
    return ticks >= SERVO_MIN_TICKS && ticks <= SERVO_MAX_TICKS && (uint32_t)ticks + SERVO_SEGMENT_MIN * SERVO_SLOW_UNITS < servo_period_units;
}

bool servo_set(uint16_t ticks)
{
    if(!servo_valid(ticks)){
        return false;
    }

    servo_plan_t plan;

    servo_plan(ticks, &plan);

    ATOMIC_BLOCK(ATOMIC_RESTORESTATE){
        servo_pending = plan;
    }
    return true;
}

bool servo_start(uint16_t ticks)
{
    if(!servo_set(ticks)){
        return false;
    }

    ATOMIC_BLOCK(ATOMIC_RESTORESTATE){
        if(servo_running){
            servo_stop_request = false;
        } else {
            servo_high = false;
            servo_left = 0;
            servo_seg = SERVO_SEGMENT_MIN;
            TCCR0A = (1 << WGM01);
            TCNT0 = 0;
            OCR0A = SERVO_SEGMENT_MIN - 1;
            TIFR0 = (1 << OCF0A);
            TIMSK0 |= (1 << OCIE0A);
            TCCR0B = SERVO_SLOW;
            servo_running = true;
        }
    }
    return true;
}

void servo_stop(void)
{
    ATOMIC_BLOCK(ATOMIC_RESTORESTATE){
        if(servo_running){
            if(servo_high){
                servo_stop_request = true;
            } else {
                servo_halt();
            }
        }
    }
}

bool servo_is_running(void)
{
    return servo_running;
}

bool servo_is_stopping(void)
{
    return servo_stop_request;
}

bool servo_is_high(void)
{
    return servo_high;
}

uint16_t servo_get_pending(void)
{
    uint16_t value;

    ATOMIC_BLOCK(ATOMIC_RESTORESTATE){
        value = servo_pending.width;
    }
    return value;
}

uint16_t servo_get_output(void)
{
    uint16_t value;

    ATOMIC_BLOCK(ATOMIC_RESTORESTATE){
        value = servo_output;
    }
    return value;
}

uint32_t servo_get_frames(void)
{
    uint32_t value;

    ATOMIC_BLOCK(ATOMIC_RESTORESTATE){
        value = servo_frames;
    }
    return value;
}

uint32_t servo_get_period_us(void)
{
    return servo_period_us;
}
