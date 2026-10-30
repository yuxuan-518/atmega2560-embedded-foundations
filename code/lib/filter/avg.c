#include "avg.h"

bool avg_init(avg_t *a, uint8_t window)
{
    if(window < 1 || window > AVG_MAX_WINDOW){
        return false;
    }

    a->window = window;
    a->count = 0;
    a->head = 0;
    a->sum = 0;
    return true;
}

uint16_t avg_push(avg_t *a, uint16_t value)
{
    if(a->count == a->window){
        a->sum -= a->buffer[a->head];
    } else {
        a->count++;
    }

    a->buffer[a->head] = value;
    a->sum += value;

    a->head++;
    if(a->head >= a->window){
        a->head = 0;
    }

    return avg_get(a);
}

uint16_t avg_get(const avg_t *a)
{
    if(a->count == 0){
        return 0;
    }

    return (uint16_t)((a->sum + a->count / 2) / a->count);
}

uint8_t avg_count(const avg_t *a)
{
    return a->count;
}

uint8_t avg_window(const avg_t *a)
{
    return a->window;
}
