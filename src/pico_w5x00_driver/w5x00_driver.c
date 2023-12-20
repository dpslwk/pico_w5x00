/*
 * Copyright (c) 2022 Raspberry Pi (Trading) Ltd.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#include "hardware/gpio.h"
#include "hardware/irq.h"
// #include "pico/binary_info.h"
#include "pico/unique_id.h"
#include "w5x00.h"
#include "w5x00_spi.h"
#include "pico/w5x00_driver.h"

#ifndef W5X00_GPIO_IRQ_HANDLER_PRIORITY
#define W5X00_GPIO_IRQ_HANDLER_PRIORITY 0x40
#endif

#ifndef W5X00_SLEEP_CHECK_MS
#define W5X00_SLEEP_CHECK_MS 50
#endif

static async_context_t *w5x00_async_context;

static void w5x00_sleep_timeout_reached(async_context_t *context, async_at_time_worker_t *worker);
static void w5x00_do_poll(async_context_t *context, async_when_pending_worker_t *worker);

static async_at_time_worker_t sleep_timeout_worker = {
        .do_work = w5x00_sleep_timeout_reached
};

static async_when_pending_worker_t w5x00_poll_worker = {
        .do_work = w5x00_do_poll
};

static void w5x00_set_irq_enabled(bool enabled) {
    gpio_set_irq_enabled(W5X00_GPIO_INTN_PIN, GPIO_IRQ_LEVEL_HIGH, enabled);
}

// GPIO interrupt handler to tell us there's w5x00 has work to do
static void w5x00_gpio_irq_handler(void)
{
    uint32_t events = gpio_get_irq_event_mask(W5X00_GPIO_INTN_PIN);
    if (events & GPIO_IRQ_LEVEL_HIGH) {
        // As we use a high level interrupt, it will go off forever until it's serviced
        // So disable the interrupt until this is done. It's re-enabled again by W5X00_POST_POLL_HOOK
        // which is called at the end of w5x00_poll_func
        w5x00_set_irq_enabled(false);
        async_context_set_work_pending(w5x00_async_context, &w5x00_poll_worker);
    }
}

uint32_t w5x00_irq_init(__unused void *param) {
#ifndef NDEBUG
    assert(get_core_num() == async_context_core_num(w5x00_async_context));
#endif
    gpio_add_raw_irq_handler_with_order_priority(W5X00_GPIO_INTN_PIN, w5x00_gpio_irq_handler, W5X00_GPIO_IRQ_HANDLER_PRIORITY);
    w5x00_set_irq_enabled(true);
    irq_set_enabled(IO_IRQ_BANK0, true);
    return 0;
}

uint32_t w5x00_irq_deinit(__unused void *param) {
#ifndef NDEBUG
    assert(get_core_num() == async_context_core_num(w5x00_async_context));
#endif
    gpio_remove_raw_irq_handler(W5X00_GPIO_INTN_PIN, w5x00_gpio_irq_handler);
    w5x00_set_irq_enabled(false);
    return 0;
}

void w5x00_post_poll_hook(void) {
#ifndef NDEBUG
    assert(get_core_num() == async_context_core_num(w5x00_async_context));
#endif
    w5x00_set_irq_enabled(true);
}

void w5x00_schedule_internal_poll_dispatch(__unused void (*func)(void)) {
    assert(func == w5x00_poll);
    async_context_set_work_pending(w5x00_async_context, &w5x00_poll_worker);
}

static void w5x00_do_poll(async_context_t *context, __unused async_when_pending_worker_t *worker) {
#ifndef NDEBUG
    assert(get_core_num() == async_context_core_num(w5x00_async_context));
#endif
    if (w5x00_poll) {
        if (w5x00_sleep > 0) {
            w5x00_sleep--;
        }
        w5x00_poll();
        if (w5x00_sleep) {
            async_context_add_at_time_worker_in_ms(context, &sleep_timeout_worker, W5X00_SLEEP_CHECK_MS);
        } else {
            async_context_remove_at_time_worker(context, &sleep_timeout_worker);
        }
    }
}

static void w5x00_sleep_timeout_reached(async_context_t *context, __unused async_at_time_worker_t *worker) {
    assert(context == w5x00_async_context);
    assert(worker == &sleep_timeout_worker);
    async_context_set_work_pending(context, &w5x00_poll_worker);
}

bool w5x00_driver_init(async_context_t *context) {
    w5x00_hal_pin_config(W5X00_GPIO_INTN_PIN, W5X00_HAL_PIN_MODE_INPUT, W5X00_HAL_PIN_PULL_UP, 0);
    // bi_decl(bi_1pin_with_name(W5X00_GPIO_INTN_PIN, "W5x00 INTERRUPT"));
    w5x00_hal_pin_config(W5X00_GPIO_RSTN_PIN, W5X00_HAL_PIN_MODE_OUTPUT, W5X00_HAL_PIN_PULL_NONE, 0);
    w5x00_hal_pin_low(W5X00_GPIO_RSTN_PIN); // Hold Wiznet in reset
    // bi_decl(bi_1pin_with_name(W5X00_GPIO_RSTN_PIN, "W5x00 RESET"));

    w5x00_state.itf_state = 0;
    w5x00_state.ethernet_link_state = W5X00_LINK_DOWN;
    w5x00_state.dma_tx = -1;
    w5x00_state.dma_rx = -1;

    w5x00_poll = NULL;
    w5x00_state.initted = true;

    w5x00_async_context = context;
    // we need the IRQ to be on the same core as the context, because we need to be able to enable/disable the IRQ
    // from there later
    async_context_execute_sync(context, w5x00_irq_init, NULL);
    async_context_add_when_pending_worker(context, &w5x00_poll_worker);
    return true;
}

void w5x00_driver_deinit(async_context_t *context) {
    assert(context == w5x00_async_context);
    async_context_remove_at_time_worker(context, &sleep_timeout_worker);
    async_context_remove_when_pending_worker(context, &w5x00_poll_worker);
    // the IRQ IS on the same core as the context, so must be de-initialized there
    async_context_execute_sync(context, w5x00_irq_deinit, NULL);
    // w5x00_deinit(&w5x00_state);  // LWK: cyw43-driver, replace with ??
    w5x00_cb_tcpip_deinit(&w5x00_state);
    w5x00_spi_deinit(&w5x00_state);

    w5x00_state.itf_state = 0;
    w5x00_state.ethernet_link_state = W5X00_LINK_DOWN;
    w5x00_poll = NULL;
    w5x00_state.initted = false;

    w5x00_async_context = NULL;
}

// Generate a mac address if one is not set in otp
void __attribute__((weak)) w5x00_hal_generate_laa_mac(__unused int idx, uint8_t buf[6]) {
    W5X00_DEBUG("Warning. No mac in otp. Generating mac from board id\n");
    pico_unique_board_id_t board_id;
    pico_get_unique_board_id(&board_id);
    memcpy(buf, &board_id.id[2], 6);
    buf[0] &= (uint8_t)~0x1; // unicast
    buf[0] |= 0x2; // locally administered
}

// Return mac address
void w5x00_hal_get_mac(__unused int idx, uint8_t buf[6]) {
    // The mac should come from w5x00 otp.
    // This is loaded into the state after the driver is initialised
    // w5x00_hal_generate_laa_mac is called by the driver to generate a mac if otp is not set
    memcpy(buf, w5x00_state.mac, 6);
}

// Prevent background processing in pensv and access by the other core
// These methods are called in pensv context and on either core
// They can be called recursively
void w5x00_thread_enter(void) {
    async_context_acquire_lock_blocking(w5x00_async_context);
}

void w5x00_thread_exit(void) {
    async_context_release_lock(w5x00_async_context);
}

#ifndef NDEBUG
void w5x00_thread_lock_check(void) {
    async_context_lock_check(w5x00_async_context);
}
#endif

void w5x00_await_background_or_timeout_us(uint32_t timeout_us) {
    async_context_wait_for_work_until(w5x00_async_context, make_timeout_time_us(timeout_us));
}

void w5x00_delay_ms(uint32_t ms) {
    async_context_wait_until(w5x00_async_context, make_timeout_time_ms(ms));
}

void w5x00_delay_us(uint32_t us) {
    async_context_wait_until(w5x00_async_context, make_timeout_time_us(us));
}

#if !W5X00_LWIP
static void no_lwip_fail() {
    panic("w5x00 has no ethernet interface");
}
void __attribute__((weak)) w5x00_cb_tcpip_init(w5x00_t *self) {
}
void __attribute__((weak)) w5x00_cb_tcpip_deinit(w5x00_t *self) {
}
void __attribute__((weak)) w5x00_cb_tcpip_set_link_up(w5x00_t *self) {
    no_lwip_fail();
}
void __attribute__((weak)) w5x00_cb_tcpip_set_link_down(w5x00_t *self) {
    no_lwip_fail();
}
void __attribute__((weak)) w5x00_cb_process_ethernet(void *cb_data, size_t len, const uint8_t *buf) {
    no_lwip_fail();
}
#endif

w5x00_t w5x00_state;
void (*w5x00_poll)(void);
uint32_t w5x00_sleep;

#ifndef W5X00_POST_POLL_HOOK
#define W5X00_POST_POLL_HOOK
#endif

static void w5x00_poll_func(void);

static int w5x00_ensure_up(w5x00_t *self) {
    W5X00_THREAD_LOCK_CHECK;

    #ifndef NDEBUG
    assert(w5x00_is_initialized(self)); // w5x00_init has not been called
    #endif
    if (w5x00_poll != NULL) {
        w5x00_ll_bus_sleep(&self->w5x00_ll, false);
        return 0;
    }

    // Disable the netif if it was previously up
    w5x00_cb_tcpip_deinit(self);
    self->itf_state = 0;

    // Reset and power up the wiznet chip
    w5x00_hal_pin_low(W5X00_GPIO_RSTN_PIN);
    w5x00_delay_ms(20);
    w5x00_hal_pin_high(W5X00_GPIO_RSTN_PIN);
    w5x00_delay_ms(50);

    // Initialise the low-level driver
    w5x00_hal_get_mac(W5X00_HAL_MAC_ETH0, self->mac);

    int ret = w5x00_ll_bus_init(&self->w5x00_ll, self->mac); // LWK ???

    if (ret != 0) {
        return ret;
    }

    W5X00_DEBUG("w5x00 loaded ok, mac %02x:%02x:%02x:%02x:%02x:%02x\n",
        self->mac[0], self->mac[1], self->mac[2], self->mac[3], self->mac[4], self->mac[5]);

    // Enable async events from low-level driver
    w5x00_sleep = W5X00_SLEEP_MAX;
    w5x00_poll = w5x00_poll_func;

    // Kick things off
    w5x00_schedule_internal_poll_dispatch(w5x00_poll_func);

    return ret;
}

// This function must always be executed at the level where W5X00_THREAD_ENTER is effectively active
static void w5x00_poll_func(void) {
    W5X00_THREAD_LOCK_CHECK;

    if (w5x00_poll == NULL) {
        // Poll scheduled during deinit, just ignore it
        return;
    }

    w5x00_t *self = &w5x00_state;

    if (w5x00_ll_has_work(&self->w5x00_ll)) {
        w5x00_ll_process_packets(&self->w5x00_ll);
    }

    // if (self->pend_disassoc) {
    //     self->pend_disassoc = false;
    //     w5x00_ll_ioctl(&self->w5x00_ll, W5X00_IOCTL_SET_DISASSOC, 0, NULL, W5X00_ITF_STA);
    // }

    // if (self->pend_rejoin_wpa) {
    //     self->pend_rejoin_wpa = false;
    //     w5x00_ll_wifi_set_wpa_auth(&self->w5x00_ll);
    // }

    // if (self->pend_rejoin) {
    //     self->pend_rejoin = false;
    //     w5x00_ll_wifi_rejoin(&self->w5x00_ll);
    //     self->wifi_join_state = WIFI_JOIN_STATE_ACTIVE;
    // }

    if (w5x00_sleep == 0) {
        w5x00_ll_bus_sleep(&self->w5x00_ll, true);
    }

    #ifdef W5X00_POST_POLL_HOOK
    W5X00_POST_POLL_HOOK
    #endif

}

int w5x00_ethernet_link_status(w5x00_t *self) {
    return self->ethernet_link_state;

    // int s = self->ethernet_link_state;
    // if (s == ) {
    //     return W5X00_LINK_JOIN;
    // } else if (s == ) {
    //     return W5X00_LINK_FAIL;
    // } else if (s == ) {
    //     return W5X00_LINK_NONET;
    // } else {
    //     return W5X00_LINK_DOWN;
    // }
}

int w5x00_send_ethernet(w5x00_t *self, size_t len, const void *buf, bool is_pbuf) {
    W5X00_THREAD_ENTER;
    int ret = w5x00_ensure_up(self);
    if (ret) {
        W5X00_THREAD_EXIT;
        return ret;
    }

    ret = w5x00_ll_send_ethernet(&self->w5x00_ll, len, buf, is_pbuf); // LWK replace with ioLibrary socket sendto ??
    W5X00_THREAD_EXIT;

    return ret;
}

void w5x00_ethernet_set_up(w5x00_t *self, bool up) {
    W5X00_THREAD_ENTER;
    if (up) {
        if (self->itf_state == 0) {
            if (w5x00_ethernet_on(self, country) != 0) {
                W5X00_THREAD_EXIT;
                return;
            }
            // w5x00_ethernet_pm(self, W5X00_DEFAULT_PM);
            w5x00_cb_tcpip_deinit(self);
            w5x00_cb_tcpip_init(self);
            self->itf_state = 1;
        }
    }
    W5X00_THREAD_EXIT;
}

int w5x00_ethernet_get_mac(w5x00_t *self, uint8_t mac[6]) {
    (void)self;
    w5x00_hal_get_mac(W5X00_HAL_MAC_ETH0, &mac[0]);
    return 0;
}

int w5x00_ethernet_join(w5x00_t *self) {
    W5X00_THREAD_ENTER;
    if (! self->itf_state) {
        W5X00_THREAD_EXIT;
        return -W5X00_EPERM;
    }

    int ret = w5x00_ensure_up(self);
    if (ret) {
        W5X00_THREAD_EXIT;
        return ret;
    }

    ret = w5x00_ll_wifi_join(self); // LWK ?????

    if (ret == 0) {
        self->ethernet_link_state = WIFI_JOIN_STATE_ACTIVE;
    }

    W5X00_THREAD_EXIT;

    return ret;
}

int w5x00_ethernet_leave(w5x00_t *self) {
    // Disassociate with SSID
    // return w5x00_ioctl(self, CYW43_IOCTL_SET_DISASSOC, 0, NULL); // LWK ????
}
