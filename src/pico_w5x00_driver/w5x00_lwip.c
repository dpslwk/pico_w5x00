
#include <string.h>

#include "w5x00.h"
#if W5X00_LWIP
#include "lwip/etharp.h"
#include "lwip/ethip6.h"
#include "lwip/dns.h"
#include "lwip/igmp.h"
#include "lwip/tcpip.h"
#include "netif/ethernet.h"
#endif

#if W5X00_LWIP


STATIC err_t w5x00_netif_output(struct netif *netif, struct pbuf *p) {
    w5x00_t *self = netif->state;
    int ret = w5x00_send_ethernet(self, p->tot_len, (void *)p, true);
    if (ret) {
        W5X00_WARN("send_ethernet failed: %d\n", ret);
        return ERR_IF;
    }
    return ERR_OK;
}

// #if LWIP_IGMP
// STATIC err_t w5x00_netif_update_igmp_mac_filter(struct netif *netif, const ip4_addr_t *group, enum netif_mac_filter_action action) {
//     w5x00_t *self = netif->state;
//     uint8_t mac[] = { 0x01, 0x00, 0x5e, ip4_addr2(group) & 0x7F, ip4_addr3(group), ip4_addr4(group) };

//     if (action != IGMP_ADD_MAC_FILTER && action != IGMP_DEL_MAC_FILTER) {
//         return ERR_VAL;
//     }

//     if (w5x00_ethernet_update_multicast_filter(self, mac, action == IGMP_ADD_MAC_FILTER)) {
//         return ERR_IF;
//     }

//     return ERR_OK;
// }
// #endif

// #if LWIP_IPV6
// STATIC err_t w5x00_macfilter(struct netif *netif, const ip6_addr_t *group, enum netif_mac_filter_action action) {
//     uint8_t address[6] = { 0x33, 0x33 };
//     memcpy(address + 2, group->addr + 3, 4);
//     if (action != NETIF_ADD_MAC_FILTER && action != NETIF_DEL_MAC_FILTER) {
//         return ERR_VAL;
//     }
//     if (w5x00_ethernet_update_multicast_filter(netif->state, address, action == NETIF_ADD_MAC_FILTER)) {
//         return ERR_IF;
//     }
//     return ERR_OK;
// }
// #endif

STATIC err_t w5x00_netif_init(struct netif *netif) {
    netif->linkoutput = w5x00_netif_output;
    #if LWIP_IPV4
    netif->output = etharp_output;
    #endif
    netif->mtu = 1500;
    netif->flags = NETIF_FLAG_BROADCAST | NETIF_FLAG_ETHARP | NETIF_FLAG_ETHERNET | NETIF_FLAG_IGMP;
    #if LWIP_IPV6
    netif->output_ip6 = ethip6_output;
    netif->mld_mac_filter = w5x00_macfilter;
    netif->flags |= NETIF_FLAG_MLD6;
    #endif
    w5x00_ethernet_get_mac(netif->state, netif->name[1] - '0', netif->hwaddr);
    netif->hwaddr_len = sizeof(netif->hwaddr);
    // #if LWIP_IGMP
    // netif_set_igmp_mac_filter(netif, w5x00_netif_update_igmp_mac_filter);
    // #endif
    return ERR_OK;
}

//#ifndef NDEBUG
//static void netif_status_callback(struct netif *netif) {
//    const ip_addr_t *ip = netif_ip_addr4(netif);
//    if (ip) {
//        W5X00_INFO("Got ip %s\n", ip4addr_ntoa(ip));
//    }
//}
//#endif

#ifndef W5X00_HOST_NAME
#define W5X00_HOST_NAME "PicoWiznet"
#endif

void w5x00_cb_tcpip_init(w5x00_t *self) {
    ip_addr_t ipconfig[4];
    #if LWIP_IPV6
    #define IP(x) ((x).u_addr.ip4)
    #else
    #define IP(x) (x)
    #endif
    #if LWIP_IPV4
    #if LWIP_DHCP
    // need to zero out to get isconnected() working
    IP4_ADDR(&IP(ipconfig[0]), 0, 0, 0, 0);
    #else
    // using static IP address
    IP(ipconfig[0]).addr = PP_HTONL(W5X00_DEFAULT_IP_ADDRESS);
    #endif
    IP(ipconfig[2]).addr = PP_HTONL(W5X00_DEFAULT_IP_GATEWAY);
    IP(ipconfig[1]).addr = PP_HTONL(W5X00_DEFAULT_IP_MASK);
    IP(ipconfig[3]).addr = PP_HTONL(W5X00_DEFAULT_IP_DNS);
    #endif
    #undef IP

    struct netif *n = &self->netif;
    n->name[0] = 'e';
    n->name[1] = '0';
    #if NO_SYS
    netif_input_fn input_func = ethernet_input;
    #else
    netif_input_fn input_func = tcpip_input;
    #endif
    #if LWIP_IPV4
    netif_add(n, ip_2_ip4(&ipconfig[0]), ip_2_ip4(&ipconfig[1]), ip_2_ip4(&ipconfig[2]), self, w5x00_netif_init, input_func);
    #elif LWIP_IPV6
    netif_add(n, self, w5x00_netif_init, input_func);
    #else
    #error Unsupported
    #endif
    netif_set_hostname(n, W5X00_HOST_NAME);
    netif_set_default(n);
    netif_set_up(n);

//    #ifndef NDEBUG
//    netif_set_status_callback(n, netif_status_callback);
//    #endif

    #if LWIP_IPV4
    #if LWIP_DNS
    dns_setserver(0, &ipconfig[3]);
    #endif
    #if LWIP_DHCP
    dhcp_set_struct(n, &self->dhcp_client);
    dhcp_start(n);
    #endif
    #endif
    #if LWIP_IPV6
    ip6_addr_t ip6_allnodes_ll;
    ip6_addr_set_allnodes_linklocal(&ip6_allnodes_ll);
    n->mld_mac_filter(n, &ip6_allnodes_ll, NETIF_ADD_MAC_FILTER);
    netif_create_ip6_linklocal_address(n, 1);
    netif_set_ip6_autoconfig_enabled(n, 1);
    #endif
}

void w5x00_cb_tcpip_deinit(w5x00_t *self) {
    struct netif *n = &self->netif;
    #if LWIP_IPV4 && LWIP_DHCP
    dhcp_stop(n);
    #endif
    for (struct netif *netif = netif_list; netif != NULL; netif = netif->next) {
        if (netif == n) {
            netif_remove(netif);
            #if LWIP_IPV4
            ip_2_ip4(&netif->ip_addr)->addr = 0;
            #endif
            netif->flags = 0;
        }
    }
}

void w5x00_cb_process_ethernet(void *cb_data, size_t len, const uint8_t *buf) {
    w5x00_t *self = cb_data;
    struct netif *netif = &self->netif;
    if (netif->flags & NETIF_FLAG_LINK_UP) {
        struct pbuf *p = pbuf_alloc(PBUF_RAW, len, PBUF_POOL);
        if (p != NULL) {
            pbuf_take(p, buf, len);
            if (netif->input(p, netif) != ERR_OK) {
                pbuf_free(p);
            }
        }
    }
}

void w5x00_cb_tcpip_set_link_up(w5x00_t *self) {
    netif_set_link_up(&self->netif);
}

void w5x00_cb_tcpip_set_link_down(w5x00_t *self) {
    netif_set_link_down(&self->netif);
}

int w5x00_tcpip_link_status(w5x00_t *self) {
    struct netif *netif = &self->netif;
    if ((netif->flags & (NETIF_FLAG_UP | NETIF_FLAG_LINK_UP)) == (NETIF_FLAG_UP | NETIF_FLAG_LINK_UP)) {
        bool have_address = false;
        #if LWIP_IPV4
        have_address = (ip_2_ip4(&netif->ip_addr)->addr != 0);
        #else
        for (int i = 0; i < LWIP_IPV6_NUM_ADDRESSES; i++) {
            int state = netif_ip6_addr_state(netif, i);
            const ip6_addr_t *addr = netif_ip6_addr(netif, i);
            if (ip6_addr_ispreferred(state) && ip6_addr_isglobal(addr)) {
                have_address = true;
                break;
            }
        }
        #endif
        if (have_address) {
            return W5X00_LINK_UP;
        } else {
            return W5X00_LINK_NOIP;
        }
    } else {
        return w5x00_ethernet_link_status(self);
    }
}

#endif //  W5X00_LWIP
