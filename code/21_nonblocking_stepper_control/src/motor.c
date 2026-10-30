#include "motor.h"
#include "timer.h"

#include <avr/io.h>
#include <util/delay.h>

static uint32_t motor_hz = MOTOR_DEFAULT_HZ;

static void motor_timer1_halt(void)
{
    // 停止Timer1
    TCCR1B &= ~((1 << CS12) |
                (1 << CS11) |
                (1 << CS10));

    // 清除未完成的计数
    TCNT1 = 0;
}

static void motor_timer1_run(void)
{
    // 恢复Timer1，64分频
    TCCR1B |= (1 << CS11) | (1 << CS10);
}

void motor_init(void)
{
    // ............ step 5/ dir 6............
    PORTB &= ~((1 << PORTB5) | (1 << PORTB6));
    DDRB |= (1 << DDB5) | (1 << DDB6);

    timer1_init(motor_hz);
}

bool motor_is_running(void)
{
    uint8_t cs_bits = TCCR1B & ((1 << CS12) |
                                (1 << CS11) |
                                (1 << CS10));
    return cs_bits != 0;
}

void motor_start(void)
{
    if(motor_is_running()){
        return;
    }

    // 清除计数
    TCNT1 = 0;
    motor_timer1_run();
}

void motor_stop(void)
{
    motor_timer1_halt();
}

bool motor_set_speed(uint32_t Hz)
{
    if(Hz < MOTOR_MIN_HZ || Hz > MOTOR_MAX_HZ){
        return false;
    }

    bool was_running = motor_is_running();

    motor_timer1_halt();

    if(!timer1_set_hz(Hz)){
        if(was_running){
            motor_timer1_run();
        }
        return false;
    }

    motor_hz = Hz;

    if(was_running){
        motor_timer1_run();
    }

    return true;
}

void motor_change_direction(void)
{
    bool was_running = motor_is_running();

    motor_timer1_halt();

    // 改变DIR
    PORTB ^= (1 << PORTB6);

    // 让DIR稳定至少5 µs
    _delay_us(10);

    if(was_running){
        motor_timer1_run();
    }
}

uint32_t motor_get_speed(void)
{
    return motor_hz;
}

uint8_t motor_get_direction(void)
{
    return (PORTB & (1 << PORTB6)) != 0;
}
