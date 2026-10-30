#include "spi.h"
#include <avr/io.h>
#include <util/atomic.h>

#define SPI_PIN_SS (1 << PORTB0)
#define SPI_PIN_SCK (1 << PORTB1)
#define SPI_PIN_MOSI (1 << PORTB2)
#define SPI_PIN_MISO (1 << PORTB3)

static bool spi_active = false;
static bool spi_use_cs = false;
static bool spi_cs_low = false;
static bool spi_cancel_request = false;
static bool spi_rx_ok = false;
static bool spi_enabled = false;
static spi_result_t spi_pending = SPI_NONE;
static uint8_t spi_owner = SPI_OWNER_NONE;
static uint8_t spi_tx[SPI_BUFFER_SIZE];
static uint8_t spi_rx[SPI_BUFFER_SIZE];
static uint8_t spi_length = 0;
static uint8_t spi_pos = 0;
static uint8_t spi_done = 0;
static uint32_t spi_byte_ms = 0;
static uint32_t spi_timeout = 2;
static spi_settings_t spi_applied = {0, 4, false};
static spi_reject_t spi_reject = SPI_REJECT_NONE;
static uint32_t spi_rejects = 0;
static uint32_t spi_ok_count = 0;
static uint32_t spi_timeout_count = 0;
static uint32_t spi_lost_count = 0;
static uint32_t spi_cancel_count = 0;

bool spi_make_settings(spi_settings_t *settings, uint8_t mode, uint32_t Hz, bool lsb_first)
{
    if(settings == 0 || mode > 3 || Hz < SPI_MIN_HZ || Hz > SPI_MAX_HZ){
        return false;
    }

    uint8_t shift = 1;

    while(shift < 7 && ((uint32_t)F_CPU >> shift) > Hz){
        shift++;
    }

    if(((uint32_t)F_CPU >> shift) > Hz){
        return false;
    }

    settings->mode = mode;
    settings->shift = shift;
    settings->lsb_first = lsb_first;
    return true;
}

bool spi_settings_valid(const spi_settings_t *settings)
{
    return settings != 0 && settings->mode <= 3 && settings->shift >= 1 && settings->shift <= 7;
}

uint32_t spi_settings_hz(const spi_settings_t *settings)
{
    if(!spi_settings_valid(settings)){
        return 0;
    }
    return (uint32_t)F_CPU >> settings->shift;
}

static void spi_pins(void)
{
    ATOMIC_BLOCK(ATOMIC_RESTORESTATE){
        if(!spi_cs_low){
            PORTB |= SPI_PIN_SS;
        }
        DDRB |= SPI_PIN_SS | SPI_PIN_SCK | SPI_PIN_MOSI;
    }
}

static void spi_apply(const spi_settings_t *settings)
{
    uint8_t spcr = (1 << SPE) | (1 << MSTR) | ((settings->mode & 3) << CPHA);
    uint8_t spsr = 0;

    if(settings->lsb_first){
        spcr |= (1 << DORD);
    }

    if(settings->shift == 7){
        spcr |= (1 << SPR1) | (1 << SPR0);
    } else {
        spcr |= ((settings->shift - 1) >> 1) << SPR0;

        if(settings->shift & 1){
            spsr = (1 << SPI2X);
        }
    }

    SPCR = spcr;
    SPSR = spsr;
    spi_applied = *settings;
    spi_enabled = true;
}

static void spi_clear_flags(void)
{
    uint8_t dummy = SPSR;
    dummy = SPDR;
    (void)dummy;
}

static void spi_release(void)
{
    if(spi_use_cs){
        PORTB |= SPI_PIN_SS;
    }
    spi_cs_low = false;
}

static void spi_disable(void)
{
    SPCR = 0;
    spi_clear_flags();
    spi_enabled = false;
}

static void spi_finish(spi_result_t result)
{
    spi_release();
    spi_active = false;
    spi_cancel_request = false;
    spi_pending = result;

    if(result == SPI_OK){
        spi_rx_ok = true;
        spi_ok_count++;
    } else if(result == SPI_TIMEOUT){
        spi_timeout_count++;
    } else if(result == SPI_MODE_LOST){
        spi_lost_count++;
    } else if(result == SPI_CANCELLED){
        spi_cancel_count++;
    }
}

static bool spi_refuse(spi_reject_t reason)
{
    spi_reject = reason;
    spi_rejects++;
    return false;
}

bool spi_init(bool miso_pullup, uint32_t timeout_ms)
{
    if(timeout_ms == 0){
        return false;
    }

    spi_timeout = timeout_ms;
    spi_active = false;
    spi_use_cs = false;
    spi_cs_low = false;
    spi_cancel_request = false;
    spi_rx_ok = false;
    spi_pending = SPI_NONE;
    spi_owner = SPI_OWNER_NONE;

    ATOMIC_BLOCK(ATOMIC_RESTORESTATE){
        PORTB |= SPI_PIN_SS;
        PORTB &= ~(SPI_PIN_SCK | SPI_PIN_MOSI);
        DDRB &= ~SPI_PIN_MISO;

        if(miso_pullup){
            PORTB |= SPI_PIN_MISO;
        } else {
            PORTB &= ~SPI_PIN_MISO;
        }
        DDRB |= SPI_PIN_SS | SPI_PIN_SCK | SPI_PIN_MOSI;
    }

    spi_settings_t settings;
    spi_make_settings(&settings, 0, 1000000UL, false);
    spi_apply(&settings);
    spi_clear_flags();
    return true;
}

bool spi_transfer(uint8_t owner, const spi_settings_t *settings, bool use_cs, const uint8_t *data, uint8_t length, uint32_t now)
{
    if(spi_active || spi_pending != SPI_NONE){
        return spi_refuse(SPI_REJECT_BUSY);
    }

    if(owner == SPI_OWNER_NONE || !spi_settings_valid(settings) || data == 0){
        return spi_refuse(SPI_REJECT_PARAM);
    }

    if(length == 0 || length > SPI_BUFFER_SIZE){
        return spi_refuse(SPI_REJECT_LENGTH);
    }

    for(uint8_t i = 0; i < length; i++){
        spi_tx[i] = data[i];
    }

    spi_owner = owner;
    spi_length = length;
    spi_pos = 0;
    spi_done = 0;
    spi_rx_ok = false;
    spi_cancel_request = false;
    spi_reject = SPI_REJECT_NONE;
    spi_use_cs = use_cs;

    spi_pins();
    spi_apply(settings);
    spi_clear_flags();

    if(use_cs){
        PORTB &= ~SPI_PIN_SS;
        spi_cs_low = true;
    }

    SPDR = spi_tx[0];
    spi_pos = 1;
    spi_byte_ms = now;
    spi_active = true;
    return true;
}

spi_result_t spi_update(uint8_t owner, uint32_t now)
{
    if(owner == SPI_OWNER_NONE || owner != spi_owner){
        return SPI_NONE;
    }

    if(spi_active){
        if(!(SPCR & (1 << MSTR))){
            spi_finish(SPI_MODE_LOST);
            spi_disable();
        } else if(SPSR & (1 << SPIF)){
            spi_rx[spi_done] = SPDR;
            spi_done++;

            if(spi_cancel_request){
                spi_finish(SPI_CANCELLED);
            } else if(spi_done >= spi_length){
                spi_finish(SPI_OK);
            } else {
                SPDR = spi_tx[spi_pos];
                spi_pos++;
                spi_byte_ms = now;
                return SPI_BUSY;
            }
        } else if(now - spi_byte_ms > spi_timeout){
            if(spi_cancel_request){
                spi_finish(SPI_CANCELLED);
            } else {
                spi_finish(SPI_TIMEOUT);
            }
            spi_disable();
        } else {
            return SPI_BUSY;
        }
    }

    spi_result_t result = spi_pending;
    spi_pending = SPI_NONE;
    return result;
}

uint8_t spi_get_data(uint8_t owner, uint8_t *data, uint8_t max)
{
    if(owner == SPI_OWNER_NONE || owner != spi_owner || spi_active || !spi_rx_ok || data == 0 || max < spi_length){
        return 0;
    }

    for(uint8_t i = 0; i < spi_length; i++){
        data[i] = spi_rx[i];
    }
    return spi_length;
}

bool spi_cancel(uint8_t owner)
{
    if(owner == SPI_OWNER_NONE || owner != spi_owner || !spi_active){
        return false;
    }

    spi_cancel_request = true;
    return true;
}

bool spi_is_busy(void)
{
    return spi_active || spi_pending != SPI_NONE;
}

bool spi_is_active(void)
{
    return spi_active;
}

bool spi_cancel_pending(void)
{
    return spi_cancel_request;
}

uint8_t spi_get_owner(void)
{
    return spi_owner;
}

uint8_t spi_get_done(void)
{
    return spi_done;
}

uint8_t spi_get_length(void)
{
    return spi_length;
}

bool spi_cs_is_low(void)
{
    return spi_cs_low;
}

bool spi_is_enabled(void)
{
    return spi_enabled;
}

void spi_get_applied(spi_settings_t *settings)
{
    if(settings != 0){
        *settings = spi_applied;
    }
}

spi_reject_t spi_get_reject(void)
{
    return spi_reject;
}

uint32_t spi_get_rejects(void)
{
    return spi_rejects;
}

uint32_t spi_get_count(spi_result_t result)
{
    if(result == SPI_OK){
        return spi_ok_count;
    }
    if(result == SPI_TIMEOUT){
        return spi_timeout_count;
    }
    if(result == SPI_MODE_LOST){
        return spi_lost_count;
    }
    if(result == SPI_CANCELLED){
        return spi_cancel_count;
    }
    return 0;
}
