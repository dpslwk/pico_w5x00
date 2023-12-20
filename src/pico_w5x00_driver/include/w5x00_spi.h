
#ifndef W5X00_INCLUDED_W5X00_SPI_H
#define W5X00_INCLUDED_W5X00_SPI_H

#include <stdint.h>
#include "w5x00.h"

static inline void w5x00_cs_select(void);
static inline void w5x00_cs_deselect(void);

static uint8_t w5x00_spi_read(void);
static void w5x00_spi_write(uint8_t tx_data);

static void w5x00_spi_read_burst(uint8_t *pBuf, uint16_t len);
static void w5x00_spi_write_burst(uint8_t *pBuf, uint16_t len);

int w5x00_spi_init(w5x00_t *self);
void w5x00_spi_deinit(w5x00_t *self);

#endif
