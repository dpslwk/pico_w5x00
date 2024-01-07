/*
 * Copyright (c) 2022 Raspberry Pi (Trading) Ltd.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#ifndef _PICO_W5X00_ARCH_ARCH_FREERTOS_H
#define _PICO_W5X00_ARCH_ARCH_FREERTOS_H

// PICO_CONFIG: W5X00_TASK_STACK_SIZE, Stack size for the W5X00 FreeRTOS task in 4-byte words, type=int, default=1024, group=pico_w5x00_arch
#ifndef W5X00_TASK_STACK_SIZE
#define W5X00_TASK_STACK_SIZE (1024 * 2)
#endif

// PICO_CONFIG: W5X00_TASK_PRIORITY, Priority for the W5X00 FreeRTOS task, type=int, default=tskIDLE_PRIORITY + 4, group=pico_w5x00_arch
#ifndef W5X00_TASK_PRIORITY
#define W5X00_TASK_PRIORITY (tskIDLE_PRIORITY + 4)
#endif

#endif
