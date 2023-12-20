/*
 * Copyright (c) 2022 Raspberry Pi (Trading) Ltd.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#if PICO_W5X00_ARCH_FREERTOS

#include "pico/w5x00_arch.h"
#include "pico/w5x00_driver.h"
#include "pico/async_context_freertos.h"

#if W5X00_LWIP
#include "pico/lwip_freertos.h"
#include <lwip/tcpip.h>
#endif

#if NO_SYS
#error example_w5x00_arch_freetos_sys requires NO_SYS=0
#endif

static async_context_freertos_t w5x00_async_context_freertos;

async_context_t *w5x00_arch_init_default_async_context(void) {
    async_context_freertos_config_t config = async_context_freertos_default_config();
#ifdef W5X00_TASK_PRIORITY
    config.task_priority = W5X00_TASK_PRIORITY;
#endif
#ifdef W5X00_TASK_STACK_SIZE
    config.task_stack_size = W5X00_TASK_STACK_SIZE;
#endif
    if (async_context_freertos_init(&w5x00_async_context_freertos, &config))
        return &w5x00_async_context_freertos.core;
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
    ok &= lwip_freertos_init(context);
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
    lwip_freertos_deinit(context);
#endif
    // if it is our context, then we de-init it.
    if (context == &w5x00_async_context_freertos.core) {
        async_context_deinit(context);
        w5x00_arch_set_async_context(NULL);
    }
}

#endif
