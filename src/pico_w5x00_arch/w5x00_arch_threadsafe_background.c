/*
 * Copyright (c) 2022 Raspberry Pi (Trading) Ltd.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#if PICO_W5X00_ARCH_THREADSAFE_BACKGROUND

#include "pico/w5x00_arch.h"
#include "pico/w5x00_driver.h"
#include "pico/async_context_threadsafe_background.h"

#if W5X00_LWIP
#include "pico/lwip_nosys.h"
#endif

#if W5X00_LWIP && !NO_SYS
#error PICO_W5X00_ARCH_THREADSAFE_BACKGROUND requires lwIP NO_SYS=1
#endif
#if W5X00_LWIP && MEM_LIBC_MALLOC
// would attempt to use malloc from IRQ context
#error MEM_LIBC_MALLOC is incompatible with PICO_W5X00_ARCH_THREADSAFE_BACKGROUND
#endif

static async_context_threadsafe_background_t w5x00_async_context_threadsafe_background;

async_context_t *w5x00_arch_init_default_async_context(void) {
    async_context_threadsafe_background_config_t config = async_context_threadsafe_background_default_config();
    if (async_context_threadsafe_background_init(&w5x00_async_context_threadsafe_background, &config))
        return &w5x00_async_context_threadsafe_background.core;
    return NULL;
}

int w5x00_arch_init(void) {
    async_context_t *context = w5x00_arch_async_context();
    if (!context) {
        context = w5x00_arch_init_default_async_context();
        if (!context) return PICO_ERROR_GENERIC;
        w5x00_arch_set_async_context(context);
    }
    bool ok = w5x00_driver_init(context);
#if W5X00_LWIP
    ok &= lwip_nosys_init(context);
#endif
    if (!ok) {
        w5x00_arch_deinit();
        return PICO_ERROR_GENERIC;
    } else {
        return 0;
    }
}

void w5x00_arch_deinit(void) {
    async_context_t *context = w5x00_arch_async_context();
    // there is a bit of a circular dependency here between lwIP and w5x00_driver. We
    // shut down w5x00_driver first as it has IRQs calling back into lwIP. Also lwIP itself
    // does not actually get shut down.
    // todo add a "pause" method to async_context if we need to provide some atomicity (we
    //      don't want to take the lock as these methods may invoke execute_sync()
    w5x00_driver_deinit(context);
#if W5X00_LWIP
    lwip_nosys_deinit(context);
#endif
    // if it is our context, then we de-init it.
    if (context == &w5x00_async_context_threadsafe_background.core) {
        async_context_deinit(context);
        w5x00_arch_set_async_context(NULL);
    }
}

#endif
