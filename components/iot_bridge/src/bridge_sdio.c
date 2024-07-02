/*
 * SPDX-FileCopyrightText: 2022-2023 Espressif Systems (Shanghai) CO LTD
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <time.h>
#include <sys/fcntl.h>
#include <sys/errno.h>
#include <sys/unistd.h>
#include <sys/select.h>
#include "lwip/opt.h"
#include "lwip/def.h"
#include "lwip/mem.h"
#include "lwip/pbuf.h"
#include "lwip/stats.h"
#include "lwip/snmp.h"
#include "lwip/ethip6.h"
#include "netif/etharp.h"

#include "esp_log.h"
#include "esp_netif.h"
#include "esp_netif_net_stack.h"

#include "esp_bridge.h"
#include "esp_bridge_internal.h"
#include "network_adapter.h"

/* Define those to better describe your network interface. */
#define IFNAME0 's'
#define IFNAME1 'd'

static const char* TAG = "bridge_sdio";

extern esp_err_t pkt_netif2driver(void *buffer, uint16_t len);
esp_netif_t* network_adapter_netif;

static esp_err_t sdio_netif_dhcp_status_change_cb(esp_ip_addr_t *ip_info)
{
    esp_dhcps_t esp_dhcps = {0};
    esp_dhcps.set_link = NIC_LINK_DOWN;
    pkt_dhcp_status_change(&esp_dhcps, sizeof(esp_dhcps_t));
    ESP_LOGW(TAG, "Down SDIO Nic");

    esp_dhcps.set_link = NIC_LINK_UP;
    pkt_dhcp_status_change(&esp_dhcps, sizeof(esp_dhcps_t));
    ESP_LOGW(TAG, "Up SDIO Nic");

    return ESP_OK;
}

static void esp_bridge_network_adapter_init(void)
{
    network_adapter_driver_init();
    ESP_LOGI(TAG, "Network Adapter initialization success");
}
struct esp_netif_lwip_vanilla_config {
    err_t (*init_fn)(struct netif*);
    void (*input_fn)(void *netif, void *buffer, size_t len, void *eb);
};

/**
 * In this function, the hardware should be initialized.
 * Invoked by ethernetif_init().
 *
 * @param netif lwip network interface which has already been initialized
 */
static void sdio_low_level_init(struct netif *netif)
{
    /* set MAC hardware address length */
    netif->hwaddr_len = ETHARP_HWADDR_LEN;

    /* maximum transfer unit */
    netif->mtu = 1500;

    /* device capabilities */
    /* don't set NETIF_FLAG_ETHARP if this device is not an ethernet one */
    netif->flags = NETIF_FLAG_BROADCAST | NETIF_FLAG_ETHARP | NETIF_FLAG_LINK_UP;

#if ESP_LWIP
#if LWIP_IGMP
    netif->flags |= NETIF_FLAG_IGMP;
#endif
#endif

#if ESP_IPV6
#if LWIP_IPV6 && LWIP_IPV6_MLD
    netif->flags |= NETIF_FLAG_MLD6;
#endif
#endif
}

/**
 * @brief This function should do the actual transmission of the packet. The packet is
 * contained in the pbuf that is passed to the function. This pbuf might be chained.
 *
 * @param netif lwip network interface structure for this ethernetif
 * @param p MAC packet to send (e.g. IP packet including MAC addresses and type)
 * @return err_t ERR_OK if the packet has been sent to Ethernet DMA buffer successfully
 *               ERR_MEM if private data couldn't be allocated
 *               ERR_IF if netif is not supported
 *               ERR_ABORT if there's some wrong when send pbuf payload to DMA buffer
 */
static err_t sdio_low_level_output(struct netif *netif, struct pbuf *p)
{
    struct pbuf *q = p;
    esp_netif_t *esp_netif = esp_netif_get_handle_from_netif_impl(netif);
    esp_err_t ret = ESP_FAIL;
    if (!esp_netif) {
        LWIP_DEBUGF(NETIF_DEBUG, ("corresponding esp-netif is NULL: netif=%p pbuf=%p len=%d\n", netif, p, p->len));
        return ERR_IF;
    }

    if (q->next == NULL) {
        ret = esp_netif_transmit(esp_netif, q->payload, q->len);
    } else {
        LWIP_DEBUGF(PBUF_DEBUG, ("low_level_output: pbuf is a list, application may has bug"));
        q = pbuf_alloc(PBUF_RAW_TX, p->tot_len, PBUF_RAM);
        if (q != NULL) {
            pbuf_copy(q, p);
        } else {
            return ERR_MEM;
        }
        ret = esp_netif_transmit(esp_netif, q->payload, q->len);
        pbuf_free(q);
    }

    if (ret == ESP_OK) {
        return ERR_OK;
    } else if (ret == ESP_ERR_NO_MEM) {
        return ERR_MEM;
    } else if (ret == ESP_ERR_INVALID_ARG) {
        return ERR_ARG;
    } else {
        return ERR_IF;
    }
}

/**
 * @brief This function should be called when a packet is ready to be read
 * from the interface. It uses the function low_level_input() that
 * should handle the actual reception of bytes from the network
 * interface. Then the type of the received packet is determined and
 * the appropriate input function is called.
 *
 * @param netif lwip network interface structure for this ethernetif
 * @param buffer ethernet buffer
 * @param len length of buffer
 */
void sdio_input(void *h, void *buffer, size_t len, void *l2_buff)
{
    struct netif * netif = h;
    esp_netif_t *esp_netif = esp_netif_get_handle_from_netif_impl(netif);
    struct pbuf *p;

    if (unlikely(!buffer || !netif_is_up(netif))) {
        if (l2_buff) {
            esp_netif_free_rx_buffer(esp_netif, l2_buff);
        }
        return;
    }

#ifdef CONFIG_LWIP_L2_TO_L3_COPY
    p = pbuf_alloc(PBUF_RAW, len, PBUF_RAM);
    if (p == NULL) {
        esp_netif_free_rx_buffer(esp_netif, l2_buff);
        return;
    }
    memcpy(p->payload, buffer, len);
    esp_netif_free_rx_buffer(esp_netif, l2_buff);
#else
    p = esp_pbuf_allocate(esp_netif, buffer, len, l2_buff);
    if (p == NULL) {
        esp_netif_free_rx_buffer(esp_netif, l2_buff);
        return;
    }

#endif

    /* full packet send to tcpip_thread to process */
    if (unlikely(netif->input(p, netif) != ERR_OK)) {
        LWIP_DEBUGF(NETIF_DEBUG, ("wlanif_input: IP input error\n"));
        pbuf_free(p);
    }
}

/**
 * Set up the network interface. It calls the function low_level_init() to do the
 * actual init work of the hardware.
 *
 * This function should be passed as a parameter to netif_add().
 *
 * @param netif the lwip network interface structure for this ethernetif
 * @return ERR_OK if the ethernetif is initialized
 */
err_t sdio_init(struct netif *netif)
{
    LWIP_ASSERT("netif != NULL", (netif != NULL));

#if LWIP_NETIF_HOSTNAME
    /* Initialize interface hostname */
#if ESP_LWIP
    if (esp_netif_get_hostname(esp_netif_get_handle_from_netif_impl(netif), &netif->hostname) != ESP_OK) {
        netif->hostname = CONFIG_LWIP_LOCAL_HOSTNAME;
    }
#else
    netif->hostname = "lwip";
#endif

#endif /* LWIP_NETIF_HOSTNAME */

    /*
    * Initialize the snmp variables and counters inside the struct netif.
    * The last argument should be replaced with your link speed, in units
    * of bits per second.
    */
    NETIF_INIT_SNMP(netif, snmp_ifType_ethernet_csmacd, 100);

    /* We directly use etharp_output() here to save a function call.
    * You can instead declare your own function an call etharp_output()
    * from it if you have to do some checks before sending (e.g. if link
    * is available...) */
    netif->output = etharp_output;
#if LWIP_IPV6
    netif->output_ip6 = ethip6_output;
#endif /* LWIP_IPV6 */
    netif->name[0] = IFNAME0;
    netif->name[1] = IFNAME1;
    netif->linkoutput = sdio_low_level_output;

    sdio_low_level_init(netif);

    return ERR_OK;
}

static void sdio_driver_free_rx_buffer(void *h, void* buffer)
{
    if (buffer) {
        free(buffer);
    }
}

static esp_err_t sdio_io_transmit(void *h, void *buffer, size_t len)
{
    // send data to driver
    pkt_netif2driver(buffer, len);
    return ESP_OK;
}

static esp_err_t sdio_io_transmit_wrap(void *h, void *buffer, size_t len, void *netstack_buf)
{
    return sdio_io_transmit(h, buffer, len);
}

static esp_netif_driver_ifconfig_t sdio_driver_ifconfig = {
    .driver_free_rx_buffer = sdio_driver_free_rx_buffer,
    .transmit = sdio_io_transmit,
    .transmit_wrap = sdio_io_transmit_wrap,
    .handle = "SDIO" // this IO object is a singleton, its handle uses as a name
};

const struct esp_netif_lwip_vanilla_config sdio_netstack_config = {
    .init_fn = sdio_init,
    .input_fn = sdio_input
};

static void sdio_got_ip_handler(void *arg, esp_event_base_t event_base,
                                int32_t event_id, void *event_data)
{
    ip_event_got_ip_t *event = (ip_event_got_ip_t*)event_data;
    esp_bridge_update_dns_info(event->esp_netif, NULL);
    ESP_LOGI(TAG, "Connected with IP Address:" IPSTR, IP2STR(&event->ip_info.ip));
}

static void sdio_lost_ip_handler(void *arg, esp_event_base_t event_base,
                                 int32_t event_id, void *event_data)
{
    IOT_BRIDGE_NAPT_TABLE_CLEAR();
}

esp_netif_t* esp_bridge_create_sdio_netif(esp_netif_ip_info_t* ip_info, uint8_t mac[6], bool data_forwarding, bool enable_dhcps)
{
    esp_netif_ip_info_t netif_ip_info = { 0 };
    const esp_netif_inherent_config_t esp_netif_common_config = {
#if defined(CONFIG_BRIDGE_DATA_FORWARDING_NETIF_SDIO)
        .flags = (esp_netif_flags_t)(ESP_NETIF_DHCP_SERVER | ESP_NETIF_FLAG_GARP | ESP_NETIF_FLAG_EVENT_IP_MODIFIED),
        .route_prio = 10,
#elif defined(CONFIG_BRIDGE_EXTERNAL_NETIF_SDIO)
        .flags = (esp_netif_flags_t)(ESP_NETIF_DHCP_CLIENT | ESP_NETIF_FLAG_GARP | ESP_NETIF_FLAG_EVENT_IP_MODIFIED),
        .route_prio = 50,
#endif
        .get_ip_event = IP_EVENT_STA_GOT_IP,
        .lost_ip_event = IP_EVENT_STA_LOST_IP,
        .if_key = "SDIO_DEF",
        .if_desc = "sdio"
    };

    esp_netif_config_t sdio_config = {
        .base = &esp_netif_common_config,
        .driver = &sdio_driver_ifconfig,
        .stack = (const esp_netif_netstack_config_t*) &sdio_netstack_config
    };

    esp_netif_t* netif = esp_bridge_create_netif(&sdio_config, ip_info, mac, enable_dhcps);
    if (netif) {
        esp_bridge_network_adapter_init();
        esp_netif_up(netif);

        if (data_forwarding) {
            esp_bridge_netif_list_add(netif, sdio_netif_dhcp_status_change_cb);
            esp_netif_get_ip_info(netif, &netif_ip_info);
            ESP_LOGI(TAG, "SDIO IP Address:" IPSTR, IP2STR(&netif_ip_info.ip));
            ip_napt_enable(netif_ip_info.ip.addr, 1);
        } else {
            esp_bridge_netif_list_add(netif, NULL);
        }

        if (enable_dhcps) {
            esp_netif_dhcps_start(netif);
        } else {
            esp_netif_dhcpc_start(netif);
            esp_event_handler_instance_register(IP_EVENT, IP_EVENT_STA_GOT_IP, &sdio_got_ip_handler, NULL, NULL);
            esp_event_handler_instance_register(IP_EVENT, IP_EVENT_STA_LOST_IP, &sdio_lost_ip_handler, NULL, NULL);
        }
    }

    network_adapter_netif = netif;
    return netif;
}
