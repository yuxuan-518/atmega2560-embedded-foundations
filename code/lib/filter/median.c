#include "median.h"

bool median_init(median_t *m, uint8_t window)
{
    if(window < 1 || window > MEDIAN_MAX_WINDOW){
        return false;
    }

    m->window = window;
    m->count = 0;
    m->head = 0;
    return true;
}

uint16_t median_push(median_t *m, uint16_t value)
{
    m->buffer[m->head] = value;
    m->head++;

    if(m->head >= m->window){
        m->head = 0;
    }

    if(m->count < m->window){
        m->count++;
    }

    return median_get(m);
}

uint16_t median_get(const median_t *m)
{
    uint16_t sorted[MEDIAN_MAX_WINDOW];

    if(m->count == 0){
        return 0;
    }

    for(uint8_t i = 0; i < m->count; i++){
        uint16_t value = m->buffer[i];
        uint8_t j = i;

        while(j > 0 && sorted[j - 1] > value){
            sorted[j] = sorted[j - 1];
            j--;
        }
        sorted[j] = value;
    }

    uint8_t middle = m->count / 2;

    if(m->count % 2 == 1){
        return sorted[middle];
    }
    return (uint16_t)(((uint32_t)sorted[middle - 1] + sorted[middle] + 1) / 2);
}

uint8_t median_count(const median_t *m)
{
    return m->count;
}
