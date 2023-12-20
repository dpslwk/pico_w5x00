/*
 * Copyright (c) 2022 Raspberry Pi (Trading) Ltd.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#include "pico/unique_id.h"
#include "w5x00.h"  // LWK: w5x00-driver, replace with ??
#include "pico/w5x00_arch.h"
#include "w5x00_ll.h"  // LWK: w5x00-driver, replace with ??

#if PICO_W5X00_ARCH_DEBUG_ENABLED
#define W5X00_ARCH_DEBUG(...) printf(__VA_ARGS__)
#else
#define W5X00_ARCH_DEBUG(...) ((void)0)
#endif

static uint32_t country_code = PICO_W5X00_ARCH_DEFAULT_COUNTRY_CODE;

static async_context_t *async_context;

void w5x00_arch_set_async_context(async_context_t *context) {
    async_context = context;
}

void w5x00_arch_enable_ethernet(void) {
    assert(w5x00_is_initialized(&w5x00_state));
    w5x00_ethernet_set_up(&w5x00_state, true);
}

void w5x00_arch_disable_ethernet(void) {
    assert(w5x00_is_initialized(&w5x00_state));
    if (w5x00_state.itf_state == 1) {
        w5x00_cb_tcpip_deinit(&w5x00_state);
        w5x00_state.itf_state = 0;
    }
    if (w5x00_state.wifi_join_state) {
        w5x00_ethernet_leave(&w5x00_state);
    }
}

#if PICO_W5X00_ARCH_DEBUG_ENABLED
// Return a string for the ethernet state
static const char* w5x00_tcpip_link_status_name(int status)
{
    switch (status) {
    case W5X00_LINK_DOWN:
        return "link down";
    case W5X00_LINK_JOIN:
        return "joining";
    case W5X00_LINK_NOIP:
        return "no ip";
    case W5X00_LINK_UP:
        return "link up";
    case W5X00_LINK_FAIL:
        return "link fail";
    case W5X00_LINK_NONET:
        return "network fail";
    case W5X00_LINK_BADAUTH:
        return "bad auth";
    }
    return "unknown";
}
#endif

int w5x00_arch_ethernet_connect_async() {
    // Connect to ethernet
    return w5x00_ethernet_join(&w5x00_state);
}

// Connect to ethernet, return with success when an IP address has been assigned
static int w5x00_arch_ethernet_connect_until(absolute_time_t until) {
    int err = w5x00_arch_ethernet_connect_async();
    if (err) return err;

    int status = W5X00_LINK_UP + 1;
    while(status >= 0 && status != W5X00_LINK_UP) {
        int new_status = w5x00_tcpip_link_status(&w5x00_state, W5X00_ITF_STA);
        // If there was no network, keep trying
        if (new_status == W5X00_LINK_NONET) {
            new_status = W5X00_LINK_JOIN;
            err = w5x00_arch_ethernet_connect_async();
            if (err) return err;
        }
        if (new_status != status) {
            status = new_status;
            W5X00_ARCH_DEBUG("connect status: %s\n", w5x00_tcpip_link_status_name(status));
        }
        if (time_reached(until)) {
            return PICO_ERROR_TIMEOUT;
        }
        // Do polling
        w5x00_arch_poll();
        w5x00_arch_wait_for_work_until(until);
    }
    // Turn status into a pico_error_codes, W5X00_LINK_NONET shouldn't happen as we fail with PICO_ERROR_TIMEOUT instead
    assert(status == W5X00_LINK_UP || status == W5X00_LINK_BADAUTH || status == W5X00_LINK_FAIL);
    if (status == W5X00_LINK_UP) {
        return PICO_OK; // success
    } else if (status == W5X00_LINK_BADAUTH) {
        return PICO_ERROR_BADAUTH;
    } else {
        return PICO_ERROR_CONNECT_FAILED;
    }
}

int w5x00_arch_ethernet_connect_blocking() {
    return w5x00_arch_ethernet_connect_until(at_the_end_of_time);
}

int w5x00_arch_ethernet_connect_timeout_ms(uint32_t timeout_ms) {
    return w5x00_arch_ethernet_connect_until(make_timeout_time_ms(timeout_ms));
}

async_context_t *w5x00_arch_async_context(void) {
    return async_context;
}

void w5x00_arch_poll(void)
{
    async_context_poll(async_context);
}

void w5x00_arch_wait_for_work_until(absolute_time_t until) {
    async_context_wait_for_work_until(async_context, until);
}
