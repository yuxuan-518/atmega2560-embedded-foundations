#include "hyst.h"

void hyst_init(hyst_t *h, uint8_t threshold)
{
    h->threshold = threshold;
    h->output = 0;
    h->valid = false;
}

void hyst_reset(hyst_t *h)
{
    h->valid = false;
}

uint8_t hyst_update(hyst_t *h, uint8_t input)
{
    if(!h->valid || h->threshold == 0){
        h->output = input;
        h->valid = true;
        return h->output;
    }

    uint8_t diff = 0;

    if(input > h->output){
        diff = input - h->output;
    } else {
        diff = h->output - input;
    }

    if(diff >= h->threshold){
        h->output = input;
    } else if((input == HYST_MIN || input == HYST_MAX) && input != h->output){
        h->output = input;
    }

    return h->output;
}

uint8_t hyst_get(const hyst_t *h)
{
    return h->output;
}
