#ifndef SPI_H
#define SPI_H

#include <stdint.h>
#include <stdbool.h>

#define SPI_MIN_HZ (F_CPU / 128UL)
#define SPI_MAX_HZ (F_CPU / 2UL)

#ifndef SPI_BUFFER_SIZE
#define SPI_BUFFER_SIZE 16
#endif

#define SPI_OWNER_NONE 0

typedef enum {
    SPI_NONE,
    SPI_BUSY,
    SPI_OK,
    SPI_TIMEOUT,
    SPI_MODE_LOST,
    SPI_CANCELLED
} spi_result_t;

typedef enum {
    SPI_REJECT_NONE,
    SPI_REJECT_BUSY,
    SPI_REJECT_LENGTH,
    SPI_REJECT_PARAM
} spi_reject_t;

typedef struct {
    uint8_t mode;
    uint8_t shift;
    bool lsb_first;
} spi_settings_t;

bool spi_make_settings(spi_settings_t *settings, uint8_t mode, uint32_t Hz, bool lsb_first);
bool spi_settings_valid(const spi_settings_t *settings);
uint32_t spi_settings_hz(const spi_settings_t *settings);

bool spi_init(bool miso_pullup, uint32_t timeout_ms);

bool spi_transfer(uint8_t owner, const spi_settings_t *settings, bool use_cs, const uint8_t *data, uint8_t length, uint32_t now);
spi_result_t spi_update(uint8_t owner, uint32_t now);
uint8_t spi_get_data(uint8_t owner, uint8_t *data, uint8_t max);
bool spi_cancel(uint8_t owner);

bool spi_is_busy(void);
bool spi_is_active(void);
bool spi_cancel_pending(void);
uint8_t spi_get_owner(void);
uint8_t spi_get_done(void);
uint8_t spi_get_length(void);
bool spi_cs_is_low(void);
bool spi_is_enabled(void);
void spi_get_applied(spi_settings_t *settings);
spi_reject_t spi_get_reject(void);
uint32_t spi_get_rejects(void);
uint32_t spi_get_count(spi_result_t result);

#endif
