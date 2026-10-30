#include "pot.h"
#include "config.h"
#include "adc.h"
#include "adc_scan.h"
#include "avg.h"
#include "calib.h"
#include "hyst.h"
#include "led.h"
#include "usart.h"

typedef struct {
    avg_t avg;
    calib_t cal;
    hyst_t hyst;
    uint32_t seen_samples;
    uint16_t avg_value;
    uint16_t cal_value;
    uint8_t map_value;
    uint8_t hold_value;
    bool ready;
} pot_channel_t;

static const uint8_t pot_channels[CONFIG_ADC_COUNT] = {
    CONFIG_ADC_CH0,
    CONFIG_ADC_CH1
};

static pot_channel_t channels[CONFIG_ADC_COUNT];
static bool scan_ready = false;
static uint8_t source = CONFIG_POT_DEFAULT_SOURCE;

uint8_t pot_raw_to_duty(uint16_t raw)
{
    if(raw > ADC_MAX_VALUE){
        raw = ADC_MAX_VALUE;
    }

    return (uint8_t)(((uint32_t)raw * 255UL) / ADC_MAX_VALUE);
}

uint32_t pot_raw_to_mv(uint16_t raw)
{
    if(raw > ADC_MAX_VALUE){
        raw = ADC_MAX_VALUE;
    }

    return ((uint32_t)raw * CONFIG_ADC_VREF_MV) / 1024UL;
}

static void pot_process(uint8_t i, bool new_sample, uint16_t raw)
{
    pot_channel_t *ch = &channels[i];

    if(new_sample){
        ch->avg_value = avg_push(&ch->avg, raw);
    }

    ch->cal_value = calib_apply(&ch->cal, ch->avg_value);
    ch->map_value = pot_raw_to_duty(ch->cal_value);
    ch->hold_value = hyst_update(&ch->hyst, ch->map_value);
    ch->ready = true;

    if(i == source){
        led_pot_input(ch->hold_value);
    }
}

static void pot_rebuild(uint8_t i)
{
    pot_channel_t *ch = &channels[i];
    const adc_scan_result_t *result = adc_scan_get(i);

    avg_init(&ch->avg, avg_window(&ch->avg));
    hyst_reset(&ch->hyst);
    ch->ready = false;

    if(scan_ready && result->valid){
        ch->seen_samples = result->samples;
        pot_process(i, true, result->raw);
    }
}

static void pot_recompute(uint8_t i)
{
    pot_channel_t *ch = &channels[i];

    hyst_reset(&ch->hyst);

    if(ch->ready){
        pot_process(i, false, 0);
    }
}

static void pot_channel_defaults(uint8_t i)
{
    pot_channel_t *ch = &channels[i];

    avg_init(&ch->avg, CONFIG_POT_WINDOW_DEFAULT);
    calib_init(&ch->cal, ADC_MAX_VALUE);
    calib_set(&ch->cal, CONFIG_POT_CAL_LOW_DEFAULT, CONFIG_POT_CAL_HIGH_DEFAULT);
    hyst_init(&ch->hyst, CONFIG_POT_HYST_DEFAULT);
}

bool pot_init(uint32_t now)
{
    for(uint8_t i = 0; i < CONFIG_ADC_COUNT; i++){
        pot_channel_defaults(i);
        channels[i].seen_samples = 0;
        channels[i].ready = false;
    }

    scan_ready = adc_scan_init(pot_channels, CONFIG_ADC_COUNT, CONFIG_ADC_SAMPLE_MS, now);
    source = CONFIG_POT_DEFAULT_SOURCE;
    return scan_ready;
}

void pot_update(uint32_t now)
{
    if(!scan_ready){
        return;
    }

    adc_scan_update(now);

    for(uint8_t i = 0; i < CONFIG_ADC_COUNT; i++){
        const adc_scan_result_t *result = adc_scan_get(i);

        if(result->valid && result->samples != channels[i].seen_samples){
            channels[i].seen_samples = result->samples;
            pot_process(i, true, result->raw);
        }
    }
}

bool pot_set_source(uint8_t new_source)
{
    if(new_source >= CONFIG_ADC_COUNT){
        return false;
    }

    source = new_source;
    return true;
}

uint8_t pot_get_source(void)
{
    return source;
}

bool pot_source_valid(void)
{
    return channels[source].ready;
}

uint8_t pot_source_duty(void)
{
    return channels[source].hold_value;
}

bool pot_set_window(uint8_t channel, uint8_t window)
{
    if(channel >= CONFIG_ADC_COUNT || window < 1 || window > AVG_MAX_WINDOW){
        return false;
    }

    avg_init(&channels[channel].avg, window);
    pot_rebuild(channel);
    return true;
}

bool pot_set_hyst(uint8_t channel, uint8_t threshold)
{
    if(channel >= CONFIG_ADC_COUNT || threshold > CONFIG_POT_HYST_MAX){
        return false;
    }

    channels[channel].hyst.threshold = threshold;
    pot_recompute(channel);
    return true;
}

bool pot_set_cal(uint8_t channel, uint16_t low, uint16_t high)
{
    if(channel >= CONFIG_ADC_COUNT){
        return false;
    }

    if(!calib_set(&channels[channel].cal, low, high)){
        return false;
    }

    pot_recompute(channel);
    return true;
}

void pot_set_default(uint8_t channel)
{
    if(channel >= CONFIG_ADC_COUNT){
        return;
    }

    pot_channel_defaults(channel);
    pot_rebuild(channel);
}

bool pot_get_view(uint8_t channel, pot_view_t *view)
{
    if(channel >= CONFIG_ADC_COUNT){
        return false;
    }

    const pot_channel_t *ch = &channels[channel];

    view->raw = pot_get_raw(channel);
    view->avg = ch->avg_value;
    view->cal = ch->cal_value;
    view->map = ch->map_value;
    view->hold = ch->hold_value;
    view->count = avg_count(&ch->avg);
    view->window = avg_window(&ch->avg);
    view->hyst = ch->hyst.threshold;
    view->cal_low = ch->cal.low;
    view->cal_high = ch->cal.high;
    view->ready = ch->ready;
    return true;
}

bool pot_is_valid(uint8_t s)
{
    const adc_scan_result_t *result = adc_scan_get(s);
    return scan_ready && result != 0 && result->valid;
}

uint16_t pot_get_raw(uint8_t s)
{
    const adc_scan_result_t *result = adc_scan_get(s);

    if(result == 0){
        return 0;
    }
    return result->raw;
}

void pot_send_status(uint32_t now)
{
    for(uint8_t i = 0; i < CONFIG_ADC_COUNT; i++){
        const adc_scan_result_t *result = adc_scan_get(i);

        usart0_send_string("ADC");
        usart0_send_uint32(i);
        usart0_send_string(" A");
        usart0_send_uint32(adc_scan_channel(i));
        usart0_send_string(": ");

        if(result->valid){
            usart0_send_string("RAW ");
            usart0_send_uint32(result->raw);
            usart0_send_string(", MV ");
            usart0_send_uint32(pot_raw_to_mv(result->raw));
            usart0_send_string(", VALID 1, AGE ");
            usart0_send_uint32(now - result->sample_ms);
            usart0_send_string(" ms");
        } else {
            usart0_send_string("RAW -, MV -, VALID 0, AGE -");
        }

        usart0_send_string(", SAMPLES ");
        usart0_send_uint32(result->samples);
        usart0_send_newline();
    }

    usart0_send_string("ADC VREF ");
    usart0_send_uint32(CONFIG_ADC_VREF_MV);
    usart0_send_string(" mV AVCC, PERIOD ");
    usart0_send_uint32(CONFIG_ADC_SAMPLE_MS);
    usart0_send_string(" ms, ROUNDS ");
    usart0_send_uint32(adc_scan_rounds());
    usart0_send_string(", DISCARDED ");
    usart0_send_uint32(adc_scan_discarded());
    usart0_send_string(", LED SOURCE A");
    usart0_send_uint32(adc_scan_channel(source));

    if(led_get_mode() == LED_POT){
        usart0_send_string(" (active)");
    } else {
        usart0_send_string(" (LED not in pot mode)");
    }
    usart0_send_newline();
}

void pot_send_filter_status(uint8_t config_channel)
{
    for(uint8_t i = 0; i < CONFIG_ADC_COUNT; i++){
        pot_view_t v;
        pot_get_view(i, &v);

        usart0_send_string("POT");
        usart0_send_uint32(i);
        usart0_send_string(": ");

        if(!v.ready){
            usart0_send_string("no sample yet");
        } else {
            usart0_send_string("RAW ");
            usart0_send_uint32(v.raw);
            usart0_send_string(", AVG ");
            usart0_send_uint32(v.avg);
            usart0_send_string(", CAL ");
            usart0_send_uint32(v.cal);
            usart0_send_string(", MAP ");
            usart0_send_uint32(v.map);
            usart0_send_string(", HOLD ");
            usart0_send_uint32(v.hold);
        }

        usart0_send_string(" | WIN ");
        usart0_send_uint32(v.count);
        usart0_send_string("/");
        usart0_send_uint32(v.window);
        usart0_send_string(", HYST ");
        usart0_send_uint32(v.hyst);
        usart0_send_string(", CAL ");
        usart0_send_uint32(v.cal_low);
        usart0_send_string("..");
        usart0_send_uint32(v.cal_high);
        usart0_send_newline();
    }

    usart0_send_string("POT CONFIG CH ");
    usart0_send_uint32(config_channel);
    usart0_send_string(", LED SOURCE ");
    usart0_send_uint32(source);

    if(led_get_mode() == LED_POT){
        usart0_send_string(" (active)");
    } else {
        usart0_send_string(" (LED not in pot mode)");
    }
    usart0_send_newline();
}
