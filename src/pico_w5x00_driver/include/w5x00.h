#ifndef W5X00_INCLUDED_W5X00_H
#define W5X00_INCLUDED_W5X00_H

#include "w5x00_config.h"
#include "hardware/dma.h"

#if W5X00_LWIP
#include "lwip/netif.h"
#include "lwip/dhcp.h"
#endif

#include <string.h>


/*!
 * \name Link status
 * \anchor W5X00_LINK_
 * \see status_name() to get a user readable name of the status for debug
 * \see w5x00_ethernet_link_status() to get the ethernet status
 * \see w5x00_tcpip_link_status() to get the overall link status
 */
//!\{
#define W5X00_LINK_DOWN         (0)     ///< link is down
#define W5X00_LINK_JOIN         (1)     ///< Connected to wifi
#define W5X00_LINK_NOIP         (2)     ///< Connected to wifi, but no IP address
#define W5X00_LINK_UP           (3)     ///< Connect to wifi with an IP address
#define W5X00_LINK_FAIL         (-1)    ///< Connection failed
#define W5X00_LINK_NONET        (-2)    ///< No matching SSID found (could be out of range, or down)
#define W5X00_LINK_BADAUTH      (-3)    ///< Authenticatation failure
//!\}

typedef struct _w5x00_t {
    int8_t dma_tx;
    int8_t dma_rx;
    dma_channel_config dma_channel_config_tx;
    dma_channel_config dma_channel_config_rx;

    uint8_t itf_state;
    uint32_t ethernet_link_state;

    uint8_t eth_frame[1514];

    bool initted;

    #if W5X00_LWIP
    // lwIP data
    struct netif netif;
    #if LWIP_IPV4 && LWIP_DHCP
    struct dhcp dhcp_client;
    #endif
    #endif

    // mac from otp (or from w5x00_hal_generate_laa_mac if not set)
    uint8_t mac[6];
} w5x00_t;

extern w5x00_t w5x00_state;
extern void (*w5x00_poll)(void);
extern uint32_t w5x00_sleep;

// void w5x00_init(w5x00_t *self);
// void w5x00_deinit(w5x00_t *self);

int w5x00_send_ethernet(w5x00_t *self, size_t len, const void *buf, bool is_pbuf);
int w5x00_ethernet_link_status(w5x00_t *self);

int w5x00_send_ethernet(w5x00_t *self, size_t len, const void *buf, bool is_pbuf);
uint16_t wiznet5k_recv_ethernet(w5x00_t *self, const uint8_t *buf);


void w5x00_ethernet_set_up(w5x00_t *self, bool up);
int w5x00_ethernet_get_mac(w5x00_t *self, uint8_t mac[6]);


int w5x00_ethernet_join(w5x00_t *self);
int w5x00_ethernet_leave(w5x00_t *self);

static inline bool w5x00_is_initialized(w5x00_t *self) {
    return self->initted;
}

void w5x00_cb_tcpip_init(w5x00_t *self);
void w5x00_cb_tcpip_deinit(w5x00_t *self);
void w5x00_cb_process_ethernet(void *cb_data, size_t len, const uint8_t *buf);
void w5x00_cb_tcpip_set_link_up(w5x00_t *self);
void w5x00_cb_tcpip_set_link_down(w5x00_t *self);
int w5x00_tcpip_link_status(w5x00_t *self);

#endif
