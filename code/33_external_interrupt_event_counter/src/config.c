#include "config.h"
#include "usart.h"
#include <avr/pgmspace.h>

#if (CONFIG_DIR_MAX_LEVEL != 0) && (CONFIG_DIR_MAX_LEVEL != 1)
#error "CONFIG_DIR_MAX_LEVEL must be 0 or 1"
#endif

#if (CONFIG_STEP_ACTIVE_LEVEL != 0) && (CONFIG_STEP_ACTIVE_LEVEL != 1)
#error "CONFIG_STEP_ACTIVE_LEVEL must be 0 or 1"
#endif

#if (CONFIG_LIMIT_ACTIVE_LEVEL != 0) && (CONFIG_LIMIT_ACTIVE_LEVEL != 1)
#error "CONFIG_LIMIT_ACTIVE_LEVEL must be 0 or 1"
#endif

#if (CONFIG_ESTOP_ACTIVE_LEVEL != 0) && (CONFIG_ESTOP_ACTIVE_LEVEL != 1)
#error "CONFIG_ESTOP_ACTIVE_LEVEL must be 0 or 1"
#endif

static config_t config = {
    CONFIG_DEFAULT_START_HZ,
    CONFIG_DEFAULT_MAX_HZ,
    CONFIG_DEFAULT_SPEED_HZ,
    CONFIG_DEFAULT_ACCEL,
    CONFIG_DEFAULT_SOFT_MIN,
    CONFIG_DEFAULT_SOFT_MAX,
    CONFIG_DEFAULT_HOME_FAST_HZ,
    CONFIG_DEFAULT_HOME_SLOW_HZ,
    CONFIG_DEFAULT_HOME_EXIT_HZ
};

static void config_warning(const char *text)
{
    usart0_send_string_P(PSTR("WARNING CONFIG "));
    usart0_send_string_P(text);
    usart0_send_newline();
}

static bool config_hz_ok(uint32_t Hz, uint32_t max_hz)
{
    return Hz >= CONFIG_HW_MIN_HZ && Hz <= max_hz;
}

static bool config_validate(const config_t *c)
{
    bool ok = true;

    if(!config_hz_ok(c->start_hz, CONFIG_HW_MAX_HZ)){
        config_warning(PSTR("start Hz must be 20-10000"));
        ok = false;
    }

    if(c->max_hz < c->start_hz || c->max_hz > CONFIG_HW_MAX_HZ){
        config_warning(PSTR("max Hz must be start-10000"));
        ok = false;
    }

    if(c->speed_hz < c->start_hz || c->speed_hz > c->max_hz){
        config_warning(PSTR("speed must be start-max"));
        ok = false;
    }

    if(c->accel > CONFIG_ACCEL_MAX){
        config_warning(PSTR("accel must be 0-50000"));
        ok = false;
    }

    if(c->soft_min < 1 || c->soft_max <= c->soft_min || c->soft_max > CONFIG_POSITION_ABS_MAX){
        config_warning(PSTR("soft limits need 1 <= softmin < softmax"));
        ok = false;
    }

    if(!config_hz_ok(c->home_fast_hz, c->max_hz) ||
       !config_hz_ok(c->home_slow_hz, c->max_hz) ||
       !config_hz_ok(c->home_exit_hz, c->max_hz)){
        config_warning(PSTR("home speeds must be 20-max"));
        ok = false;
    }

    return ok;
}

void config_init(void)
{
}

const config_t *config_get(void)
{
    return &config;
}

bool config_check(void)
{
    return config_validate(&config);
}

bool config_set(config_item_t item, int32_t value)
{
    config_t candidate = config;

    if(value < 0 && item != CONFIG_SOFT_MIN && item != CONFIG_SOFT_MAX){
        config_warning(PSTR("value must be positive"));
        return false;
    }

    switch(item){
        case CONFIG_START:
            candidate.start_hz = value;
            break;
        case CONFIG_MAX:
            candidate.max_hz = value;
            break;
        case CONFIG_SPEED:
            candidate.speed_hz = value;
            break;
        case CONFIG_ACCEL:
            candidate.accel = value;
            break;
        case CONFIG_SOFT_MIN:
            candidate.soft_min = value;
            break;
        case CONFIG_SOFT_MAX:
            candidate.soft_max = value;
            break;
    }

    if(!config_validate(&candidate)){
        return false;
    }

    config = candidate;
    return true;
}

static void config_send_int32(int32_t number)
{
    if(number < 0){
        usart0_send_string_P(PSTR("-"));
        number = -number;
    }
    usart0_send_uint32(number);
}

void config_send(void)
{
    usart0_send_string_P(PSTR("CONFIG start "));
    usart0_send_uint32(config.start_hz);
    usart0_send_string_P(PSTR(", max "));
    usart0_send_uint32(config.max_hz);
    usart0_send_string_P(PSTR(", speed "));
    usart0_send_uint32(config.speed_hz);
    usart0_send_string_P(PSTR(" Hz, accel "));
    usart0_send_uint32(config.accel);
    usart0_send_string_P(PSTR(" Hz/s"));
    usart0_send_newline();

    usart0_send_string_P(PSTR("CONFIG soft "));
    config_send_int32(config.soft_min);
    usart0_send_string_P(PSTR(".."));
    config_send_int32(config.soft_max);
    usart0_send_string_P(PSTR(", home "));
    usart0_send_uint32(config.home_fast_hz);
    usart0_send_string_P(PSTR("/"));
    usart0_send_uint32(config.home_slow_hz);
    usart0_send_string_P(PSTR("/"));
    usart0_send_uint32(config.home_exit_hz);
    usart0_send_string_P(PSTR(" Hz, move max "));
    usart0_send_uint32(CONFIG_MOVE_MAX_STEPS);
    usart0_send_newline();

    usart0_send_string_P(PSTR("CONFIG level dir "));
    usart0_send_uint32(CONFIG_DIR_MAX_LEVEL);
    usart0_send_string_P(PSTR(", step "));
    usart0_send_uint32(CONFIG_STEP_ACTIVE_LEVEL);
    usart0_send_string_P(PSTR(", limit "));
    usart0_send_uint32(CONFIG_LIMIT_ACTIVE_LEVEL);
    usart0_send_string_P(PSTR(", estop "));
    usart0_send_uint32(CONFIG_ESTOP_ACTIVE_LEVEL);
    usart0_send_newline();
}
