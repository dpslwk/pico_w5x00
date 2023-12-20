/*
 * Copyright (c) 2022 Raspberry Pi (Trading) Ltd.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#ifndef _PICO_W5X00_ARCH_H
#define _PICO_W5X00_ARCH_H

#include "pico.h"

#ifdef __cplusplus
extern "C" {
#endif

#include "w5x00.h"                  // LWK: w5x00-driver, replace with ioLibrary glue?
#include "pico/async_context.h"

#ifdef PICO_W5X00_ARCH_HEADER
#include __XSTRING(PICO_W5X00_ARCH_HEADER)
#else
#if PICO_W5X00_ARCH_POLL
#include "pico/w5x00_arch/arch_poll.h"
#elif PICO_W5X00_ARCH_THREADSAFE_BACKGROUND
#include "pico/w5x00_arch/arch_threadsafe_background.h"
#elif PICO_W5X00_ARCH_FREERTOS
#include "pico/w5x00_arch/arch_freertos.h"
#else
#error must specify support pico_w5x00_arch architecture type or set PICO_W5X00_ARCH_HEADER
#endif
#endif

/**
 * \defgroup w5x00_driver w5x00_driver
 * \ingroup pico_w5x00_arch
 * \brief Driver used for Pico W wireless
*/

/**
 * \defgroup w5x00_ll w5x00_ll
 * \ingroup w5x00_driver
 * \brief Low Level W5X00 driver interface
*/

/** \file pico/w5x00_arch.h
 *  \defgroup pico_w5x00_arch pico_w5x00_arch
 *
 * Architecture for integrating the W5X00 driver (for the wireless on Pico W) and lwIP (for TCP/IP stack) into the SDK. It is also necessary for accessing the on-board LED on Pico W
 *
 * Both the low level \c w5x00_driver and the lwIP stack require periodic servicing, and have limitations
 * on whether they can be called from multiple cores/threads.
 *
 * \c pico_w5x00_arch attempts to abstract these complications into several behavioral groups:
 *
 * * \em 'poll' - This not multi-core/IRQ safe, and requires the user to call \ref w5x00_arch_poll periodically from their main loop
 * * \em 'thread_safe_background' - This is multi-core/thread/task safe, and maintenance of the driver and TCP/IP stack is handled automatically in the background
 * * \em 'freertos' - This is multi-core/thread/task safe, and uses a separate FreeRTOS task to handle lwIP and and driver work.
 *
 * As of right now, lwIP is the only supported TCP/IP stack, however the use of \c pico_w5x00_arch is intended to be independent of
 * the particular TCP/IP stack used (and possibly Bluetooth stack used) in the future. For this reason, the integration of lwIP
 * is handled in the base (\c pico_w5x00_arch) library based on the #define \ref W5X00_LWIP used by the \c w5x00_driver.
 *
 * \note As of version 1.5.0 of the Raspberry Pi Pico SDK, the \c pico_w5x00_arch library no longer directly implements
 * the distinct behavioral abstractions. This is now handled by the more general \ref pico_async_context library. The
 * user facing behavior of pico_w5x00_arch has not changed as a result of this implementation detail, however pico_w5x00_arch
 * is now just a thin wrapper which creates an appropriate async_context and makes a simple call to add lwIP or w5x00_driver support
 * as appropriate. You are free to perform this context creation and adding of lwIP, w5x00_driver or indeed any other additional
 * future protocol/driver support to your async_context, however for now pico_w5x00_arch does still provide a few w5x00_ specific (i.e. Pico W)
 * APIs for connection management, locking and GPIO interaction.
 *
 * \note The connection management APIs at least may be moved
 * to a more generic library in a future release. The locking methods are now backed by their \ref pico_async_context equivalents, and
 * those methods may be used interchangeably (see \ref w5x00_arch_lwip_begin, \ref w5x00_arch_lwip_end and \ref w5x00_arch_lwip_check for more details).
 *
 * \note For examples of creating of your own async_context and addition of \c w5x00_driver and \c lwIP support, please
 * refer to the specific source files \c w5x00_arch_poll.c, \c w5x00_arch_threadsafe_background.c and \c w5x00_arch_freertos.c.
 *
 * Whilst you can use the \c pico_w5x00_arch library directly and specify \ref W5X00_LWIP (and other defines) yourself, several
 * other libraries are made available to the build which aggregate the defines and other dependencies for you:
 *
 * * \b pico_w5x00_arch_lwip_poll - For using the RAW lwIP API (in `NO_SYS=1` mode) without any background processing or multi-core/thread safety.
 *
 *    The user must call \ref w5x00_arch_poll periodically from their main loop.
 *
 *    This wrapper library:
 *    - Sets \c W5X00_LWIP=1 to enable lwIP support in \c pico_w5x00_arch and \c w5x00_driver.
 *    - Sets \c PICO_W5X00_ARCH_POLL=1 to select the polling behavior.
 *    - Adds the \c pico_lwip as a dependency to pull in lwIP.
 *
 * * \b pico_w5x00_arch_lwip_threadsafe_background - For using the RAW lwIP API (in `NO_SYS=1` mode) with multi-core/thread safety, and automatic servicing of the \c w5x00_driver and
 * lwIP in background.
 *
 *    Calls into the \c w5x00_driver high level API (w5x00.h) may be made from either core or from lwIP callbacks, however calls into lwIP (which
 * is not thread-safe) other than those made from lwIP callbacks, must be bracketed with \ref w5x00_arch_lwip_begin and \ref w5x00_arch_lwip_end. It is fine to bracket
 * calls made from within lwIP callbacks too; you just don't have to.
 *
 *    \note lwIP callbacks happen in a (low priority) IRQ context (similar to an alarm callback), so care should be taken when interacting
 *    with other code.
 *
 *    This wrapper library:
 *    - Sets \c W5X00_LWIP=1 to enable lwIP support in \c pico_w5x00_arch and \c w5x00_driver
 *    - Sets \c PICO_W5X00_ARCH_THREADSAFE_BACKGROUND=1 to select the thread-safe/non-polling behavior.
 *    - Adds the pico_lwip as a dependency to pull in lwIP.
 *
 *
 *    This library \em can also be used under the RP2040 port of FreeRTOS with lwIP in `NO_SYS=1` mode (allowing you to call \c w5x00_driver APIs
 * from any task, and to call lwIP from lwIP callbacks, or from any task if you bracket the calls with \ref w5x00_arch_lwip_begin and \ref w5x00_arch_lwip_end. Again, you should be
 * careful about what you do in lwIP callbacks, as you cannot call most FreeRTOS APIs from within an IRQ context. Unless you have good reason, you should probably
 * use the full FreeRTOS integration (with `NO_SYS=0`) provided by \c pico_w5x00_arch_lwip_sys_freertos.
 *
 * * \b pico_w5x00_arch_lwip_sys_freertos - For using the full lwIP API including blocking sockets in OS (`NO_SYS=0`) mode, along with with multi-core/task/thread safety, and automatic servicing of the \c w5x00_driver and
 * the lwIP stack.
 *
 *    This wrapper library:
 *    - Sets \c W5X00_LWIP=1 to enable lwIP support in \c pico_w5x00_arch and \c w5x00_driver.
 *    - Sets \c PICO_W5X00_ARCH_FREERTOS=1 to select the NO_SYS=0 lwip/FreeRTOS integration
 *    - Sets \c LWIP_PROVIDE_ERRNO=1 to provide error numbers needed for compilation without an OS
 *    - Adds the \c pico_lwip as a dependency to pull in lwIP.
 *    - Adds the lwIP/FreeRTOS code from lwip-contrib (in the contrib directory of lwIP)
 *
 *    Calls into the \c w5x00_driver high level API (w5x00.h) may be made from any task or from lwIP callbacks, but not from IRQs. Calls into the lwIP RAW API (which is not thread safe)
 *    must be bracketed with \ref w5x00_arch_lwip_begin and \ref w5x00_arch_lwip_end. It is fine to bracket calls made from within lwIP callbacks too; you just don't have to.
 *
 *    \note this wrapper library requires you to link FreeRTOS functionality with your application yourself.
 *
 * * \b pico_w5x00_arch_none - If you do not need the TCP/IP stack but wish to use the on-board LED.
 *
 *    This wrapper library:
 *    - Sets \c W5X00_LWIP=0 to disable lwIP support in \c pico_w5x00_arch and \c w5x00_driver
 */

// PICO_CONFIG: PARAM_ASSERTIONS_ENABLED_W5X00_ARCH, Enable/disable assertions in the pico_w5x00_arch module, type=bool, default=0, group=pico_w5x00_arch
#ifndef PARAM_ASSERTIONS_ENABLED_W5X00_ARCH
#define PARAM_ASSERTIONS_ENABLED_W5X00_ARCH 0
#endif

// PICO_CONFIG: PICO_W5X00_ARCH_DEBUG_ENABLED, Enable/disable some debugging output in the pico_w5x00_arch module, type=bool, default=1 in debug builds, group=pico_w5x00_arch
#ifndef PICO_W5X00_ARCH_DEBUG_ENABLED
#ifndef NDEBUG
#define PICO_W5X00_ARCH_DEBUG_ENABLED 1
#else
#define PICO_W5X00_ARCH_DEBUG_ENABLED 0
#endif
#endif

/*!
 * \brief Initialize the W5X00 architecture
 * \ingroup pico_w5x00_arch
 *
 * This method initializes the `w5x00_driver` code and initializes the lwIP stack (if it
 * was enabled at build time). This method must be called prior to using any other \c pico_w5x00_arch,
 * \c w5x00_driver or lwIP functions.
 *
 * \note this method initializes wireless with a country code of \c PICO_W5X00_ARCH_DEFAULT_COUNTRY_CODE
 * which defaults to \c W5X00_COUNTRY_WORLDWIDE. Worldwide settings may not give the best performance; consider
 * setting PICO_W5X00_ARCH_DEFAULT_COUNTRY_CODE to a different value or calling \ref w5x00_arch_init_with_country
 *
 * By default this method initializes the w5x00_arch code's own async_context by calling
 * \ref w5x00_arch_init_default_async_context, however the user can specify use of their own async_context
 * by calling \ref w5x00_arch_set_async_context() before calling this method
 *
 * \return 0 if the initialization is successful, an error code otherwise \see pico_error_codes
 */
int w5x00_arch_init(void);

/*!
 * \brief De-initialize the W5X00 architecture
 * \ingroup pico_w5x00_arch
 *
 * This method de-initializes the `w5x00_driver` code and de-initializes the lwIP stack (if it
 * was enabled at build time). Note this method should always be called from the same core (or RTOS
 * task, depending on the environment) as \ref w5x00_arch_init.
 *
 * Additionally if the w5x00_arch is using its own async_context instance, then that instance is de-initialized.
 */
void w5x00_arch_deinit(void);

/*!
 * \brief Return the current async_context currently in use by the w5x00_arch code
 * \ingroup pico_w5x00_arch
 *
 * \return the async_context.
 */
async_context_t *w5x00_arch_async_context(void);

/*!
 * \brief Set the async_context to be used by the w5x00_arch_init
 * \ingroup pico_w5x00_arch
 *
 * \note This method must be called before calling w5x00_arch_init or w5x00_arch_init_with_country
 * if you wish to use a custom async_context instance.
 *
 * \param context the async_context to be used
 */
void w5x00_arch_set_async_context(async_context_t *context);

/*!
 * \brief Initialize the default async_context for the current w5x00_arch type
 * \ingroup pico_w5x00_arch
 *
 * This method initializes and returns a pointer to the static async_context associated
 * with w5x00_arch. This method is called by \ref w5x00_arch_init automatically
 * if a different async_context has not been set by \ref w5x00_arch_set_async_context
 *
 * \return the context or NULL if initialization failed.
 */
async_context_t *w5x00_arch_init_default_async_context(void);

/*!
 * \brief Perform any processing required by the \c w5x00_driver or the TCP/IP stack
 * \ingroup pico_w5x00_arch
 *
 * This method must be called periodically from the main loop when using a
 * \em polling style \c pico_w5x00_arch (e.g. \c pico_w5x00_arch_lwip_poll ). It
 * may be called in other styles, but it is unnecessary to do so.
 */
void w5x00_arch_poll(void);

/*!
 * \brief Sleep until there is w5x00_driver work to be done
 * \ingroup pico_w5x00_arch
 *
 * This method may be called by code that is waiting for an event to
 * come from the w5x00_driver, and has no work to do, but would like
 * to sleep without blocking any background work associated with the w5x00_driver.
 *
 * \param until the time to wait until if there is no work to do.
 */
void w5x00_arch_wait_for_work_until(absolute_time_t until);

/*!
 * \fn w5x00_arch_lwip_begin
 * \brief Acquire any locks required to call into lwIP
 * \ingroup pico_w5x00_arch
 *
 * The lwIP API is not thread safe. You should surround calls into the lwIP API
 * with calls to this method and \ref w5x00_arch_lwip_end. Note these calls are not
 * necessary (but harmless) when you are calling back into the lwIP API from an lwIP callback.
 * If you are using single-core polling only (pico_w5x00_arch_poll) then these calls are no-ops
 * anyway it is good practice to call them anyway where they are necessary.
 *
 * \note as of SDK release 1.5.0, this is now equivalent to calling \ref async_context_acquire_lock_blocking
 * on the async_context associated with w5x00_arch and lwIP.
 *
 * \sa w5x00_arch_lwip_end
 * \sa w5x00_arch_lwip_protect
 * \sa async_context_acquire_lock_blocking
 * \sa w5x00_arch_async_context
 */
static inline void w5x00_arch_lwip_begin(void) {
    w5x00_thread_enter();
}

/*!
 * \fn void w5x00_arch_lwip_end(void)
 * \brief Release any locks required for calling into lwIP
 * \ingroup pico_w5x00_arch
 *
 * The lwIP API is not thread safe. You should surround calls into the lwIP API
 * with calls to \ref w5x00_arch_lwip_begin and this method. Note these calls are not
 * necessary (but harmless) when you are calling back into the lwIP API from an lwIP callback.
 * If you are using single-core polling only (pico_w5x00_arch_poll) then these calls are no-ops
 * anyway it is good practice to call them anyway where they are necessary.
 *
 * \note as of SDK release 1.5.0, this is now equivalent to calling \ref async_context_release_lock
 * on the async_context associated with w5x00_arch and lwIP.
 *
 * \sa w5x00_arch_lwip_begin
 * \sa w5x00_arch_lwip_protect
 * \sa async_context_release_lock
 * \sa w5x00_arch_async_context
 */
static inline void w5x00_arch_lwip_end(void) {
    w5x00_thread_exit();
}

/*!
 * \fn int w5x00_arch_lwip_protect(int (*func)(void *param), void *param)
 * \brief sad Release any locks required for calling into lwIP
 * \ingroup pico_w5x00_arch
 *
 * The lwIP API is not thread safe. You can use this method to wrap a function
 * with any locking required to call into the lwIP API. If you are using
 * single-core polling only (pico_w5x00_arch_poll) then there are no
 * locks to required, but it is still good practice to use this function.
 *
 * \param func the function ta call with any required locks held
 * \param param parameter to pass to \c func
 * \return the return value from \c func
 * \sa w5x00_arch_lwip_begin
 * \sa w5x00_arch_lwip_end
 */
static inline int w5x00_arch_lwip_protect(int (*func)(void *param), void *param) {
    w5x00_arch_lwip_begin();
    int rc = func(param);
    w5x00_arch_lwip_end();
    return rc;
}

/*!
 * \fn void w5x00_arch_lwip_check(void)
 * \brief Checks the caller has any locks required for calling into lwIP
 * \ingroup pico_w5x00_arch
 *
 * The lwIP API is not thread safe. You should surround calls into the lwIP API
 * with calls to \ref w5x00_arch_lwip_begin and this method. Note these calls are not
 * necessary (but harmless) when you are calling back into the lwIP API from an lwIP callback.
 *
 * This method will assert in debug mode, if the above conditions are not met (i.e. it is not safe to
 * call into the lwIP API)
 *
 * \note as of SDK release 1.5.0, this is now equivalent to calling \ref async_context_lock_check
 * on the async_context associated with w5x00_arch and lwIP.
 *
 * \sa w5x00_arch_lwip_begin
 * \sa w5x00_arch_lwip_protect
 * \sa async_context_lock_check
 * \sa w5x00_arch_async_context
 */


/*!
 * \brief Enables Wi-Fi STA (Station) mode.
 * \ingroup pico_w5x00_arch
 *
 * This enables the Wi-Fi in \em Station mode such that connections can be made to other Wi-Fi Access Points
 */
void w5x00_arch_enable_ethernet(void);

/*!
 * \brief Disables Wi-Fi STA (Station) mode.
 * \ingroup pico_w5x00_arch
 *
 * This disables the Wi-Fi in \em Station mode, disconnecting any active connection.
 * You should subsequently check the status by calling \ref w5x00_ethernet_link_status.
 */
void w5x00_arch_disable_ethernet(void);

/*!
 * \brief Attempt to connect to a wireless access point, blocking until the network is joined or a failure is detected.
 * \ingroup pico_w5x00_arch
 *
 * \return 0 if the initialization is successful, an error code otherwise \see pico_error_codes
 */
int w5x00_arch_ethernet_connect_blocking();

/*!
 * \brief Attempt to connect to a wireless access point, blocking until the network is joined, a failure is detected or a timeout occurs
 * \ingroup pico_w5x00_arch
 *
 * \param timeout how long to wait in milliseconds for a connection to succeed before giving up
 *
 * \return 0 if the initialization is successful, an error code otherwise \see pico_error_codes
 */
int w5x00_arch_ethernet_connect_timeout_ms(uint32_t timeout);

/*!
 * \brief Start attempting to connect to a wireless access point
 * \ingroup pico_w5x00_arch
 *
 * This method tells the W5X00 driver to start connecting to an access point. You should subsequently check the
 * status by calling \ref w5x00_ethernet_link_status.
 *
 * \return 0 if the scan was started successfully, an error code otherwise \see pico_error_codes
 */
int w5x00_arch_ethernet_connect_async();

#ifdef __cplusplus
}
#endif

#endif
