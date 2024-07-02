/*
 * SPDX-FileCopyrightText: 2022 Espressif Systems (Shanghai) CO LTD
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
#include "esp_mac.h"
#include "esp_netif.h"
#include "esp_netif_net_stack.h"

#include "esp_bridge_internal.h"
#include "tinyusb.h"
#if ESP_IDF_VERSION < ESP_IDF_VERSION_VAL(5, 1, 4)
#include "tusb_net.h"
#include "tusb_bth.h"
#else
#include "tinyusb_net.h"
#endif

#if CFG_TUD_BTH
#if ESP_IDF_VERSION >= ESP_IDF_VERSION_VAL(5, 1, 4)
#error "USB BTH currently only support in IDF versions 5.0~5.1.3"
#endif
#endif

/* Define those to better describe your network interface. */
#define IFNAME0 'u'
#define IFNAME1 's'

static const char* TAG = "bridge_usb";

#if ESP_IDF_VERSION < ESP_IDF_VERSION_VAL(5, 1, 4)
esp_err_t pkt_netif2usb(void *buffer, uint16_t len);
#endif
esp_netif_t* usb_netif;

static void usb_lost_ip_handler(void *arg, esp_event_base_t event_base,
                                int32_t event_id, void *event_data)
{
    IOT_BRIDGE_NAPT_TABLE_CLEAR();
}

static esp_err_t usb_netif_dhcp_status_change_cb(esp_ip_addr_t *ip_info)
{
#ifdef CONFIG_TINYUSB_NET_ECM
    if (tud_connected()) {
        ecm_close();
        ecm_open();
    }
#else
    ESP_LOGE(TAG, "You need to reset the USB Nic to get the new IP");
    ESP_LOGE(TAG, "If you want automatic reset, please use USB CDC-ECM");
#endif
    return ESP_OK;
}

#if ESP_IDF_VERSION >= ESP_IDF_VERSION_VAL(5, 1, 4)
static esp_err_t usb_recv_callback(void *buffer, uint16_t len, void *ctx)
{
    // ESP_LOG_BUFFER_HEXDUMP(" usb ==> netif", src, size, ESP_LOG_INFO);
    esp_netif_receive(usb_netif, buffer, len, NULL);
    return ESP_OK;
}
#endif

static void esp_bridge_usb_init(void)
{
    ESP_LOGI(TAG, "USB device initialization");
#if ESP_IDF_VERSION >= ESP_IDF_VERSION_VAL(5, 1, 4)
    const tinyusb_config_t tusb_cfg = {
        .external_phy = false,
    };
    ESP_ERROR_CHECK(tinyusb_driver_install(&tusb_cfg));

    tinyusb_net_config_t net_config = {
        .on_recv_callback = usb_recv_callback,
    };
    esp_read_mac(net_config.mac_addr, ESP_MAC_WIFI_STA);
    uint8_t *mac = net_config.mac_addr;
    ESP_LOGI(TAG, "Network interface HW address: %02x:%02x:%02x:%02x:%02x:%02x", mac[0], mac[1], mac[2], mac[3], mac[4], mac[5]);
    ESP_ERROR_CHECK(tinyusb_net_init(TINYUSB_USBDEV_0, &net_config));
#else
    tusb_net_init();

#if CFG_TUD_BTH
    // init ble controller
    tusb_bth_init();
#endif /* CFG_TUD_BTH */

    tinyusb_config_t tusb_cfg = {
        .external_phy = false // In the most cases you need to use a `false` value
    };

    ESP_ERROR_CHECK(tinyusb_driver_install(&tusb_cfg));
#endif
    ESP_LOGI(TAG, "USB initialization DONE");
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
static void usb_low_level_init(struct netif *netif)
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
 * @return err_t ERR_OK if the packet has been sent to buffer successfully
 *               ERR_MEM if private data couldn't be allocated
 *               ERR_IF if netif is not supported
 *               ERR_ABORT if there's some wrong when send pbuf payload to DMA buffer
 */
static err_t usb_low_level_output(struct netif *netif, struct pbuf *p)
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
void usb_input(void *h, void *buffer, size_t len, void *l2_buff)
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
err_t usb_init(struct netif *netif)
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
    netif->linkoutput = usb_low_level_output;

    usb_low_level_init(netif);

    return ERR_OK;
}

static void usb_driver_free_rx_buffer(void *h, void* buffer)
{
    if (buffer) {
        free(buffer);
    }
}

#if ESP_IDF_VERSION >= ESP_IDF_VERSION_VAL(5, 1, 4)
static esp_err_t pkt_netif2usb(void *buffer, uint16_t len)
{
    tinyusb_net_send_sync(buffer, len, NULL, portMAX_DELAY);
    return ESP_OK;
}
#endif

static esp_err_t usb_io_transmit(void *h, void *buffer, size_t len)
{
    // send data to usb driver
    pkt_netif2usb(buffer, len);
    return ESP_OK;
}

static esp_err_t usb_io_transmit_wrap(void *h, void *buffer, size_t len, void *netstack_buf)
{
    return usb_io_transmit(h, buffer, len);
}

static esp_netif_driver_ifconfig_t usb_driver_ifconfig = {
    .driver_free_rx_buffer = usb_driver_free_rx_buffer,
    .transmit = usb_io_transmit,
    .transmit_wrap = usb_io_transmit_wrap,
    .handle = "USB" // this IO object is a singleton, its handle uses as a name
};

static const struct esp_netif_lwip_vanilla_config usb_netstack_config = {
    .init_fn = usb_init,
    .input_fn = usb_input
};

esp_netif_t* esp_bridge_create_usb_netif(esp_netif_ip_info_t* ip_info, uint8_t mac[6], bool data_forwarding, bool enable_dhcps)
{
    esp_netif_ip_info_t netif_ip_info = { 0 };
    const esp_netif_inherent_config_t esp_netif_common_config = {
        .flags = (ESP_NETIF_DHCP_SERVER | ESP_NETIF_FLAG_GARP | ESP_NETIF_FLAG_EVENT_IP_MODIFIED),
        .get_ip_event = IP_EVENT_STA_GOT_IP,
        .lost_ip_event = IP_EVENT_STA_LOST_IP,
        .if_key = "USB_DEF",
        .if_desc = "usb"
    };

    esp_netif_config_t usb_config = {
        .base = &esp_netif_common_config,
        .driver = &usb_driver_ifconfig,
        .stack = (const esp_netif_netstack_config_t*) &usb_netstack_config
    };

    if (!data_forwarding) {
        return NULL;
    }

    esp_netif_t* netif = esp_bridge_create_netif(&usb_config, ip_info, mac, enable_dhcps);
    if (netif) {
        esp_bridge_usb_init();
        esp_bridge_netif_list_add(netif, usb_netif_dhcp_status_change_cb);
        if (data_forwarding) {
            esp_netif_get_ip_info(netif, &netif_ip_info);
            ESP_LOGI(TAG, "USB IP Address:" IPSTR, IP2STR(&netif_ip_info.ip));
            ip_napt_enable(netif_ip_info.ip.addr, 1);
        } else {
            esp_event_handler_instance_register(IP_EVENT, IP_EVENT_STA_LOST_IP, &usb_lost_ip_handler, NULL, NULL);
        }
    }

    usb_netif = netif;
    return netif;
}
