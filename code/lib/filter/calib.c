#include "calib.h"

bool calib_init(calib_t *c, uint16_t max)
{
    if(max == 0){
        return false;
    }

    c->max = max;
    c->low = 0;
    c->high = max;
    return true;
}

bool calib_set(calib_t *c, uint16_t low, uint16_t high)
{
    if(low >= high || high > c->max){
        return false;
    }

    c->low = low;
    c->high = high;
    return true;
}

uint16_t calib_apply(const calib_t *c, uint16_t value)
{
    if(value <= c->low){
        return 0;
    }

    if(value >= c->high){
        return c->max;
    }

    uint32_t span = c->high - c->low;
    uint32_t offset = value - c->low;

    return (uint16_t)((offset * c->max) / span);
}
