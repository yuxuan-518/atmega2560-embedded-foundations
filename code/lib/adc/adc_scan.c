#include "adc_scan.h"
#include "adc.h"

#include <string.h>

typedef enum {
    SCAN_IDLE,
    SCAN_START,
    SCAN_WAIT
} scan_state_t;

static uint8_t scan_channels[ADC_SCAN_MAX_CHANNELS];
static adc_scan_result_t scan_results[ADC_SCAN_MAX_CHANNELS];
static uint8_t scan_count = 0;
static uint32_t scan_period = 0;

static scan_state_t state = SCAN_IDLE;
static uint8_t scan_index = 0;
static bool discard = true;
static uint8_t last_channel = 0xFF;
static uint32_t round_start = 0;
static uint32_t rounds = 0;
static uint32_t discarded = 0;
static uint32_t mismatch = 0;

bool adc_scan_init(const uint8_t channels[], uint8_t count, uint32_t period_ms, uint32_t now)
{
    if(count == 0 || count > ADC_SCAN_MAX_CHANNELS || period_ms == 0){
        return false;
    }

    for(uint8_t i = 0; i < count; i++){
        if(channels[i] > ADC_MAX_CHANNEL){
            return false;
        }
    }

    adc_init();

    for(uint8_t i = 0; i < count; i++){
        scan_channels[i] = channels[i];
        adc_disable_digital(channels[i]);
    }

    memset(scan_results, 0, sizeof(scan_results));
    scan_count = count;
    scan_period = period_ms;
    state = SCAN_IDLE;
    scan_index = 0;
    discard = true;
    last_channel = 0xFF;
    round_start = now - period_ms;
    rounds = 0;
    discarded = 0;
    mismatch = 0;
    return true;
}

bool adc_scan_update(uint32_t now)
{
    if(scan_count == 0){
        return false;
    }

    if(state == SCAN_IDLE){
        if(now - round_start < scan_period){
            return false;
        }

        round_start = now;
        scan_index = 0;
        state = SCAN_START;
    }

    if(state == SCAN_START){
        uint8_t channel = scan_channels[scan_index];

        if(!adc_start(channel)){
            return false;
        }

        discard = (channel != last_channel);
        last_channel = channel;
        state = SCAN_WAIT;
        return false;
    }

    uint16_t value = 0;
    uint8_t owner = 0;

    if(!adc_read_channel(&value, &owner)){
        return false;
    }

    if(owner != scan_channels[scan_index]){
        mismatch++;
        state = SCAN_START;
        return false;
    }

    if(discard){
        discarded++;
        state = SCAN_START;
        return false;
    }

    adc_scan_result_t *result = &scan_results[scan_index];
    result->raw = value;
    result->valid = true;
    result->sample_ms = now;
    result->samples++;

    scan_index++;

    if(scan_index >= scan_count){
        rounds++;
        state = SCAN_IDLE;
    } else {
        state = SCAN_START;
    }

    return true;
}

uint8_t adc_scan_count(void)
{
    return scan_count;
}

uint8_t adc_scan_channel(uint8_t i)
{
    if(i >= scan_count){
        return 0xFF;
    }
    return scan_channels[i];
}

const adc_scan_result_t *adc_scan_get(uint8_t i)
{
    if(i >= scan_count){
        return 0;
    }
    return &scan_results[i];
}

uint32_t adc_scan_rounds(void)
{
    return rounds;
}

uint32_t adc_scan_discarded(void)
{
    return discarded;
}

uint32_t adc_scan_mismatch(void)
{
    return mismatch;
}

uint32_t adc_scan_period(void)
{
    return scan_period;
}
