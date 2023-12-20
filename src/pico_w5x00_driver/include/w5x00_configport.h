/*
 * Copyright (c) 2022 Raspberry Pi (Trading) Ltd.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

// This header is included by w5x00_driver to setup its environment

#ifndef _W5X00_CONFIGPORT_H
#define _W5X00_CONFIGPORT_H

#include "pico.h"
#include "hardware/gpio.h"
#include "pico/time.h"

#ifdef __cplusplus
extern "C" {
#endif

#ifndef W5X00_GPIO_INTN_PIN
#define W5X00_GPIO_INTN_PIN 21
#endif

#ifndef W5X00_GPIO_RSTN_PIN
#define W5X00_GPIO_RSTN_PIN 20
#endif

#ifndef W5X00_SPI_PORT
#define W5X00_SPI_PORT spi0
#endif
#ifndef W5X00_SPI_SCK_PIN
#define W5X00_SPI_SCK_PIN 18
#endif
#ifndef W5X00_SPI_MOSI_PIN
#define W5X00_SPI_MOSI_PIN 19
#endif
#ifndef W5X00_SPI_MISO_PIN
#define W5X00_SPI_MISO_PIN 16
#endif
#ifndef W5X00_SPI_CSN_PIN
#define W5X00_SPI_CSN_PIN 17
#endif

#ifndef W5X00_HOST_NAME
#define W5X00_HOST_NAME "PicoWiznet"
#endif

#ifndef W5X00_IOCTL_TIMEOUT_US
#define W5X00_IOCTL_TIMEOUT_US 1000000
#endif

// todo should this be user settable?
// *** LWK *** __unused
#ifndef W5X00_HAL_MAC_ETH0
#define W5X00_HAL_MAC_ETH0 0
#endif

#ifndef STATIC
#define STATIC static
#endif

// Note, these are negated, because w5x00_driver negates them before returning!
#define W5X00_EPERM            (-PICO_ERROR_NOT_PERMITTED) // Operation not permitted
#define W5X00_EIO              (-PICO_ERROR_IO) // I/O error
#define W5X00_EINVAL           (-PICO_ERROR_INVALID_ARG) // Invalid argument
#define W5X00_ETIMEDOUT        (-PICO_ERROR_TIMEOUT) // Connection timed out

#define w5x00_hal_pin_obj_t uint

// get the number of elements in a fixed-size array
#define W5X00_ARRAY_SIZE(a) count_of(a)

static inline uint32_t w5x00_hal_ticks_us(void) {
    return time_us_32();
}

static inline uint32_t w5x00_hal_ticks_ms(void) {
    return to_ms_since_boot(get_absolute_time());
}

static inline int w5x00_hal_pin_read(w5x00_hal_pin_obj_t pin) {
    return gpio_get(pin);
}

static inline void w5x00_hal_pin_low(w5x00_hal_pin_obj_t pin) {
    gpio_clr_mask(1 << pin);
}

static inline void w5x00_hal_pin_high(w5x00_hal_pin_obj_t pin) {
    gpio_set_mask(1 << pin);
}

#define W5X00_HAL_PIN_MODE_INPUT           (GPIO_IN)
#define W5X00_HAL_PIN_MODE_OUTPUT          (GPIO_OUT)

#define W5X00_HAL_PIN_PULL_NONE            (0)
#define W5X00_HAL_PIN_PULL_UP              (1)
#define W5X00_HAL_PIN_PULL_DOWN            (2)

static inline void w5x00_hal_pin_config(w5x00_hal_pin_obj_t pin, uint32_t mode, uint32_t pull, __unused uint32_t alt) {
    assert((mode == W5X00_HAL_PIN_MODE_INPUT || mode == W5X00_HAL_PIN_MODE_OUTPUT) && alt == 0);
    gpio_set_dir(pin, mode);
    gpio_set_pulls(pin, pull == W5X00_HAL_PIN_PULL_UP, pull == W5X00_HAL_PIN_PULL_DOWN);
}

void w5x00_hal_get_mac(int idx, uint8_t buf[6]);

void w5x00_hal_generate_laa_mac(int idx, uint8_t buf[6]);


void w5x00_thread_enter(void);

void w5x00_thread_exit(void);

#define W5X00_THREAD_ENTER w5x00_thread_enter();
#define W5X00_THREAD_EXIT w5x00_thread_exit();
#ifndef NDEBUG

void w5x00_thread_lock_check(void);

#define w5x00_arch_lwip_check() w5x00_thread_lock_check()
#define W5X00_THREAD_LOCK_CHECK w5x00_arch_lwip_check();
#else
#define w5x00_arch_lwip_check() ((void)0)
#define W5X00_THREAD_LOCK_CHECK
#endif

void w5x00_await_background_or_timeout_us(uint32_t timeout_us);
// todo not 100% sure about the timeouts here; MP uses __WFI which will always wakeup periodically
#define W5X00_SDPCM_SEND_COMMON_WAIT w5x00_await_background_or_timeout_us(1000);
#define W5X00_DO_IOCTL_WAIT w5x00_await_background_or_timeout_us(1000);

void w5x00_delay_ms(uint32_t ms);

void w5x00_delay_us(uint32_t us);

void w5x00_schedule_internal_poll_dispatch(void (*func)(void));

void w5x00_post_poll_hook(void);

#define W5X00_POST_POLL_HOOK w5x00_post_poll_hook();

// Allow malloc and free to be changed
#ifndef w5x00_malloc
#define w5x00_malloc malloc
#endif
#ifndef w5x00_free
#define w5x00_free free
#endif

#ifdef __cplusplus
}
#endif


#endif
