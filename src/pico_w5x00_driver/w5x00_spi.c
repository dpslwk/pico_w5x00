

#include <stdio.h>

#include "pico/stdlib.h"
// #include "pico/binary_info.h"
#include "hardware/spi.h"
#include "hardware/dma.h"
#include "w5x00.h"

static inline void w5x00_cs_select(void)
{
    w5x00_hal_pin_low(W5X00_SPI_CSN_PIN);
}

static inline void w5x00_cs_deselect(void)
{
    w5x00_hal_pin_high(W5X00_SPI_CSN_PIN);
}

static uint8_t w5x00_spi_read(void)
{
    uint8_t rx_data = 0;
    uint8_t tx_data = 0xFF;

    spi_read_blocking(W5X00_SPI, tx_data, &rx_data, 1);

    return rx_data;
}

static void w5x00_spi_write(uint8_t tx_data)
{
    spi_write_blocking(W5X00_SPI, &tx_data, 1);
}

static void w5x00_spi_read_burst(uint8_t *pBuf, uint16_t len)
{
    uint8_t dummy_data = 0xFF;

    channel_config_set_read_increment(&w5x00_state.dma_channel_config_tx, false);
    channel_config_set_write_increment(&w5x00_state.dma_channel_config_tx, false);
    dma_channel_configure(w5x00_state.dma_tx, &w5x00_state.dma_channel_config_tx,
                          &spi_get_hw(W5X00_SPI)->dr, // write address
                          &dummy_data,               // read address
                          len,                       // element count (each element is of size transfer_data_size)
                          false);                    // don't start yet

    channel_config_set_read_increment(&w5x00_state.dma_channel_config_rx, false);
    channel_config_set_write_increment(&w5x00_state.dma_channel_config_rx, true);
    dma_channel_configure(w5x00_state.dma_rx, &w5x00_state.dma_channel_config_rx,
                          pBuf,                      // write address
                          &spi_get_hw(W5X00_SPI)->dr, // read address
                          len,                       // element count (each element is of size transfer_data_size)
                          false);                    // don't start yet

    dma_start_channel_mask((1u << w5x00_state.dma_tx) | (1u << w5x00_state.dma_rx));
    // dma_channel_wait_for_finish_blocking(w5x00_state.dma_rx);
    while(dma_channel_is_busy(w5x00_state.dma_rx)) {
      w5x00_delay_ms(1);
    }
}

static void w5x00_spi_write_burst(uint8_t *pBuf, uint16_t len)
{
    uint8_t dummy_data;

    channel_config_set_read_increment(&w5x00_state.dma_channel_config_tx, true);
    channel_config_set_write_increment(&w5x00_state.dma_channel_config_tx, false);
    dma_channel_configure(w5x00_state.dma_tx, &w5x00_state.dma_channel_config_tx,
                          &spi_get_hw(W5X00_SPI)->dr, // write address
                          pBuf,                      // read address
                          len,                       // element count (each element is of size transfer_data_size)
                          false);                    // don't start yet

    channel_config_set_read_increment(&w5x00_state.dma_channel_config_rx, false);
    channel_config_set_write_increment(&w5x00_state.dma_channel_config_rx, false);
    dma_channel_configure(w5x00_state.dma_rx, &w5x00_state.dma_channel_config_rx,
                          &dummy_data,               // write address
                          &spi_get_hw(W5X00_SPI)->dr, // read address
                          len,                       // element count (each element is of size transfer_data_size)
                          false);                    // don't start yet

    dma_start_channel_mask((1u << w5x00_state.dma_tx) | (1u << w5x00_state.dma_rx));
    // dma_channel_wait_for_finish_blocking(w5x00_state.dma_rx);
    while(dma_channel_is_busy(w5x00_state.dma_rx)) {
      w5x00_delay_ms(1);
    }
}

int w5x00_spi_init(w5x00_int_t *self)
{
    // this example will use SPI0 at 5MHz
    spi_init(SPI_PORT, 5000 * 1000);

    gpio_set_function(W5X00_SPI_SCK_PIN, GPIO_FUNC_SPI);
    gpio_set_function(W5X00_SPI_MOSI_PIN, GPIO_FUNC_SPI);
    gpio_set_function(W5X00_SPI_MISO_PIN, GPIO_FUNC_SPI);

    // make the SPI pins available to picotool
    // bi_decl(bi_3pins_with_func(W5X00_SPI_MISO_PIN, W5X00_SPI_MOSI_PIN, W5X00_SPI_SCK_PIN, GPIO_FUNC_SPI));

    // chip select is active-low, so we'll initialise it to a driven-high state
    w5x00_hal_pin_config(W5X00_SPI_CSN_PIN, W5X00_HAL_PIN_MODE_OUTPUT, W5X00_HAL_PIN_PULL_NONE, 0);
    w5x00_hal_pin_high(W5X00_SPI_CSN_PIN);

    // make the SPI pins available to picotool
    // bi_decl(bi_1pin_with_name(W5X00_SPI_CSN_PIN, "W5x00 CHIP SELECT"));

    self->dma_tx = -1;
    self->dma_rx = -1;

    self->dma_tx = (int8_t) dma_claim_unused_channel(false);
    self->dma_rx = (int8_t) dma_claim_unused_channel(false);

    if (self->dma_tx < 0 || self->dma_rx < 0) {
        return W5X00_FAIL_FAST_CHECK(-W5X00_EIO);
    }

    self->dma_channel_config_tx = dma_channel_get_default_config(self->dma_tx);
    channel_config_set_transfer_data_size(&self->dma_channel_config_tx, DMA_SIZE_8);
    channel_config_set_dreq(&self->dma_channel_config_tx, DREQ_SPI0_TX);

    // We set the inbound DMA to transfer from the SPI receive FIFO to a memory buffer paced by the SPI RX FIFO DREQ
    // We configure the read address to remain unchanged for each element, but the write
    // address to increment (so data is written throughout the buffer)
    self->dma_channel_config_rx = dma_channel_get_default_config(self->dma_rx);
    channel_config_set_transfer_data_size(&self->dma_channel_config_rx, DMA_SIZE_8);
    channel_config_set_dreq(&self->dma_channel_config_rx, DREQ_SPI0_RX);
    channel_config_set_read_increment(&self->dma_channel_config_rx, false);
    channel_config_set_write_increment(&self->dma_channel_config_rx, true);

    return 0;
}

void w5x00_spi_deinit(w5x00_t *self)
{
    if (self->dma_tx >= 0) {
        dma_channel_cleanup(self->dma_tx);
        dma_channel_unclaim(self->dma_tx);
        self->dma_tx = -1;
    }
    if (self->dma_rx >= 0) {
        dma_channel_cleanup(self->dma_rx);
        dma_channel_unclaim(self->dma_rx);
        self->dma_rx = -1;
    }
}
