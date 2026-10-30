#include "limit.h"
#include "config.h"
#include <avr/io.h>

#define LIMIT_MIN 0
#define LIMIT_MAX 1

static bool limit_active[2] = {false, false};
static uint32_t limit_last_active[2] = {0, 0};

bool limit_min_raw(void)
{
    uint8_t level = (PINK & (1 << PINK0)) != 0;
    return level == CONFIG_LIMIT_ACTIVE_LEVEL;
}

bool limit_max_raw(void)
{
    uint8_t level = (PINK & (1 << PINK1)) != 0;
    return level == CONFIG_LIMIT_ACTIVE_LEVEL;
}

void limit_init(void)
{
    DDRK &= ~((1 << DDK0) | (1 << DDK1));
    PORTK |= (1 << PORTK0) | (1 << PORTK1);

    PCMSK2 |= (1 << PCINT16) | (1 << PCINT17);
    PCIFR = (1 << PCIF2);
    PCICR |= (1 << PCIE2);
}

void limit_update(uint32_t now)
{
    bool raw[2];
    raw[LIMIT_MIN] = limit_min_raw();
    raw[LIMIT_MAX] = limit_max_raw();

    for(uint8_t i = 0; i < 2; i++){
        if(raw[i]){
            limit_active[i] = true;
            limit_last_active[i] = now;
        } else if(limit_active[i] && now - limit_last_active[i] >= CONFIG_LIMIT_RELEASE_MS){
            limit_active[i] = false;
        }
    }
}

bool limit_min_active(void)
{
    return limit_active[LIMIT_MIN];
}

bool limit_max_active(void)
{
    return limit_active[LIMIT_MAX];
}

bool limit_any_active(void)
{
    return limit_active[LIMIT_MIN] || limit_active[LIMIT_MAX];
}
