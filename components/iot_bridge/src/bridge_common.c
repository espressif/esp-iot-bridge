/*
 * SPDX-FileCopyrightText: 2022 Espressif Systems (Shanghai) CO LTD
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <string.h>
#include <sys/fcntl.h>
#include <sys/errno.h>
#include <sys/unistd.h>
#include <sys/select.h>

#include "esp_log.h"
#include "esp_netif.h"
#include "esp_system.h"
#if ESP_IDF_VERSION >= ESP_IDF_VERSION_VAL(4, 4, 0)
#include "esp_mac.h"
#endif

#include "lwip/inet.h"
#include "lwip/ip_addr.h"
#include "dhcpserver/dhcpserver.h"

#include "esp_bridge.h"
#include "esp_bridge_wifi.h"
#include "esp_bridge_internal.h"

// DHCP_Server has to be enabled for this netif
#define DHCPS_NETIF_ID(netif) (ESP_NETIF_DHCP_SERVER & esp_netif_get_flags(netif))

typedef struct bridge_netif {
    esp_netif_t *netif;
    dhcps_change_cb_t dhcps_change_cb;
    struct bridge_netif *next;
} bridge_netif_t;

static const char *TAG = "bridge_common";
static bridge_netif_t *bridge_link = NULL;

esp_err_t _esp_bridge_netif_list_add(esp_netif_t *netif, dhcps_change_cb_t dhcps_change_cb, const char *commit_id)
{
    bridge_netif_t *new = bridge_link;
    bridge_netif_t *tail = NULL;

    while (new) {
        if (new->netif == netif) {
            return ESP_ERR_DUPLICATE_ADDITION;
        }

        tail = new;
        new = new->next;
    }

    // not found, create a new
    new = (bridge_netif_t *)malloc(sizeof(bridge_netif_t));

    if (new == NULL) {
        ESP_LOGE(TAG, "netif list add fail");
        return ESP_ERR_NO_MEM;
    }

    printf("Add netif %s with %s(commit id)\r\n", esp_netif_get_desc(netif), commit_id);
    new->netif = netif;
    new->dhcps_change_cb = dhcps_change_cb;
    new->next = NULL;

    if (tail == NULL) { // the first one
        bridge_link = new;
    } else {
        tail->next = new;
    }

    ESP_LOGI(TAG, "netif list add success");

    return ESP_OK;
}

esp_err_t esp_bridge_netif_list_remove(esp_netif_t *netif)
{
    bridge_netif_t *current = bridge_link;
    bridge_netif_t *prev = NULL;

    while (current) {
        if (current->netif == netif) {
            if (prev == NULL) {
                bridge_link = bridge_link->next;
            } else {
                prev->next = current->next;
            }

            free(current);
            break;
        }

        prev = current;
        current = current->next;
    }

    return ESP_OK;
}

static bool esp_bridge_netif_network_segment_is_used(uint32_t ip)
{
    bridge_netif_t *p = bridge_link;
    esp_netif_ip_info_t netif_ip = { 0 };

    while (p) {
        esp_netif_get_ip_info(p->netif, &netif_ip);

        if (esp_ip4_addr3_16((esp_ip4_addr_t *)&ip) == esp_ip4_addr3_16(&netif_ip.ip)) {
            return true;
        }

        p = p->next;
    }

    return false;
}

typedef bool (*esp_bridge_network_segment_custom_check_cb_t)(uint32_t ip);

typedef struct esp_bridge_network_segment_custom_check_type {
    esp_bridge_network_segment_custom_check_cb_t custom_check_cb;
    struct esp_bridge_network_segment_custom_check_type *next;
} esp_bridge_network_segment_custom_check_t;
static esp_bridge_network_segment_custom_check_t *custom_check_list;

bool esp_bridge_network_segment_check_register(bool (*custom_check_cb)(uint32_t ip))
{
    esp_bridge_network_segment_custom_check_t *list = (esp_bridge_network_segment_custom_check_t *)malloc(sizeof(esp_bridge_network_segment_custom_check_t));

    if (list) {
        memset(list, 0x0, sizeof(esp_bridge_network_segment_custom_check_t));
        list->custom_check_cb = custom_check_cb;
        list->next = custom_check_list;
        custom_check_list = list;
    }

    return true;
}

esp_err_t esp_bridge_netif_request_ip(esp_netif_ip_info_t *ip_info)
{
    bool ip_segment_is_used = true;

    for (uint8_t bridge_ip = 4; bridge_ip < 255; bridge_ip++) {
        esp_bridge_network_segment_custom_check_t *list = custom_check_list;
        ip_segment_is_used = esp_bridge_netif_network_segment_is_used(ESP_IP4TOADDR(192, 168, bridge_ip, 1));

        while (!ip_segment_is_used && list) {
            ip_segment_is_used = list->custom_check_cb(ESP_IP4TOADDR(192, 168, bridge_ip, 1));
            list = list->next;
        }

        if (!ip_segment_is_used) {
            ip_info->ip.addr = ESP_IP4TOADDR(192, 168, bridge_ip, 1);
            ip_info->gw.addr = ESP_IP4TOADDR(192, 168, bridge_ip, 1);
            ip_info->netmask.addr = ESP_IP4TOADDR(255, 255, 255, 0);
            ESP_LOGI("ip select", "IP Address:" IPSTR, IP2STR(&ip_info->ip));
            ESP_LOGI("ip select", "GW Address:" IPSTR, IP2STR(&ip_info->gw));
            ESP_LOGI("ip select", "NM Address:" IPSTR, IP2STR(&ip_info->netmask));

            return ESP_OK;
        }
    }

    return ESP_FAIL;
}

static bool esp_bridge_netif_mac_is_used(uint8_t mac[6])
{
    bridge_netif_t *p = bridge_link;
    uint8_t netif_mac[6] = { 0 };

    while (p) {
        esp_netif_get_mac(p->netif, netif_mac);

        if (!memcmp(netif_mac, mac, sizeof(netif_mac))) {
            return true;
        }

        p = p->next;
    }

    return false;
}

esp_err_t esp_bridge_netif_request_mac(uint8_t *mac)
{
    uint8_t netif_mac[6] = { 0 };
    esp_read_mac(netif_mac, ESP_MAC_WIFI_STA);

    while (1) {
        if (!esp_bridge_netif_mac_is_used(netif_mac)) {
            break;
        }

        if (netif_mac[5] != 0xff) {
            if (netif_mac[4] != 0xff) {
                netif_mac[3] += 1;
            }

            netif_mac[4] += 1;
        }

        netif_mac[5] += 1;
    }

    memcpy(mac, netif_mac, sizeof(netif_mac));
    ESP_LOGI("mac select", "MAC "MACSTR"", MAC2STR(mac));
    return ESP_OK;
}

esp_err_t esp_bridge_netif_network_segment_conflict_update(esp_netif_t* esp_netif)
{
    bridge_netif_t* p = bridge_link;
    esp_netif_ip_info_t netif_ip;
    esp_netif_ip_info_t allocate_ip_info;
    esp_ip_addr_t esp_ip_addr_info;
    esp_ip4_addr_t netmask = { .addr = ESP_IP4TOADDR(255, 255, 255, 0) };
    bool ip_segment_is_used = false;

    memset(&allocate_ip_info, 0x0, sizeof(esp_netif_ip_info_t));

    while (p) {
        esp_bridge_network_segment_custom_check_t* list = custom_check_list;
        if ((esp_netif != p->netif) && DHCPS_NETIF_ID(p->netif)) { /* DHCP_Server has to be enabled for this netif */
            if (esp_netif) {
                esp_netif_get_ip_info(esp_netif, &allocate_ip_info);
            }
            esp_netif_get_ip_info(p->netif, &netif_ip);

            while (list) {
                ip_segment_is_used = list->custom_check_cb(netif_ip.ip.addr);
                list = list->next;
            }
            /* The checked network segment does not conflict with the external netif */
            /* And the same ip net segment is not be used by other external netifs */
            if ((!ip4_addr_netcmp(&netif_ip.ip, &allocate_ip_info.ip, &netmask)) && !ip_segment_is_used) {
                p = p->next;
                continue;
            }

            if (esp_bridge_netif_request_ip(&allocate_ip_info) != ESP_OK) {
                ESP_LOGE(TAG, "ip reallocate fail");
                break;
            }

            ESP_ERROR_CHECK(esp_netif_dhcps_stop(p->netif));
            esp_netif_set_ip_info(p->netif, &allocate_ip_info);
            ESP_LOGI(TAG, "ip reallocate new:" IPSTR, IP2STR(&allocate_ip_info.ip));
            ESP_ERROR_CHECK(esp_netif_dhcps_start(p->netif));

            esp_ip_addr_info.type = ESP_IPADDR_TYPE_V4;
            esp_ip_addr_info.u_addr.ip4.addr = allocate_ip_info.ip.addr;

            if (p->dhcps_change_cb) {
                p->dhcps_change_cb(&esp_ip_addr_info);
            }

            break;
        }
        p = p->next;
    }
    return ESP_OK;
}

esp_netif_t* esp_bridge_create_netif(esp_netif_config_t* config, esp_netif_ip_info_t* custom_ip_info, uint8_t custom_mac[6], bool enable_dhcps)
{
    esp_netif_ip_info_t allocate_ip_info = { 0 };
    uint8_t allocate_mac[6] = { 0 };
    esp_netif_t* netif = esp_netif_new(config);
    assert(netif);

    esp_netif_dhcps_stop(netif);
    if (custom_ip_info) { // Custom IP
        esp_netif_set_ip_info(netif, custom_ip_info);
    } else {
        if (enable_dhcps) {
            esp_bridge_netif_request_ip(&allocate_ip_info);
            esp_netif_set_ip_info(netif, &allocate_ip_info);
        }
    }

    if (custom_mac) { // Custom MAC
        ESP_ERROR_CHECK(esp_netif_set_mac(netif, custom_mac));
    } else {
        if (enable_dhcps) {
            esp_bridge_netif_request_mac(allocate_mac);
            esp_netif_set_mac(netif, allocate_mac);
        }
    }
    // Start the netif in a manual way, no need for events
    esp_netif_action_start(netif, NULL, 0, NULL);
    esp_netif_up(netif);

    if (enable_dhcps) {
        esp_netif_dns_info_t dns;
        dns.ip.u_addr.ip4.addr = ESP_IP4TOADDR(114, 114, 114, 114);
        dns.ip.type = IPADDR_TYPE_V4;
        dhcps_offer_t dhcps_dns_value = OFFER_DNS;
        ESP_ERROR_CHECK(esp_netif_dhcps_option(netif, ESP_NETIF_OP_SET, ESP_NETIF_DOMAIN_NAME_SERVER, &dhcps_dns_value, sizeof(dhcps_dns_value)));
        ESP_ERROR_CHECK(esp_netif_set_dns_info(netif, ESP_NETIF_DNS_MAIN, &dns));
        ESP_ERROR_CHECK(esp_netif_dhcps_start(netif));
    }

    return netif;
}

void esp_bridge_create_all_netif(void)
{
    ESP_LOGI(TAG, "esp-iot-bridge version: %d.%d.%d", IOT_BRIDGE_VER_MAJOR, IOT_BRIDGE_VER_MINOR, IOT_BRIDGE_VER_PATCH);

#if defined(CONFIG_BRIDGE_DATA_FORWARDING_NETIF_SOFTAP)
    esp_bridge_create_softap_netif(NULL, NULL, true, true);
#if defined(CONFIG_BRIDGE_WIFI_PMF_DISABLE)
    esp_wifi_disable_pmf_config(WIFI_IF_AP);
#endif
#endif

#if defined(CONFIG_BRIDGE_DATA_FORWARDING_NETIF_USB)
    esp_bridge_create_usb_netif(NULL, NULL, true, true);
#endif

#if defined(CONFIG_BRIDGE_DATA_FORWARDING_NETIF_SPI)
    esp_bridge_create_spi_netif(NULL, NULL, true, true);
#elif defined(CONFIG_BRIDGE_EXTERNAL_NETIF_SPI)
    uint8_t spi_mac[6] = {0x1, 0x2, 0x3, 0x4, 0x5, 0x6};
    esp_bridge_create_spi_netif(NULL, spi_mac, false, false);
#endif

#if defined(CONFIG_BRIDGE_DATA_FORWARDING_NETIF_SDIO)
    esp_bridge_create_sdio_netif(NULL, NULL, true, true);
#elif defined(CONFIG_BRIDGE_EXTERNAL_NETIF_SDIO)
    uint8_t sdio_mac[6] = {0x1, 0x2, 0x3, 0x4, 0x5, 0x6};
    esp_bridge_create_sdio_netif(NULL, sdio_mac, false, false);
#endif

#if defined(CONFIG_BRIDGE_DATA_FORWARDING_NETIF_ETHERNET)
    esp_bridge_create_eth_netif(NULL, NULL, true, true);
#elif defined(CONFIG_BRIDGE_EXTERNAL_NETIF_ETHERNET)
    esp_bridge_create_eth_netif(NULL, NULL, false, false);
#endif

#if ESP_IDF_VERSION >= ESP_IDF_VERSION_VAL(4, 4, 0)
#if defined(CONFIG_BRIDGE_EXTERNAL_NETIF_MODEM)
    esp_bridge_create_modem_netif(NULL, NULL, false, false);
#endif
#endif

#if defined(CONFIG_BRIDGE_EXTERNAL_NETIF_STATION)
    esp_bridge_create_station_netif(NULL, NULL, false, false);
#if defined(CONFIG_BRIDGE_WIFI_PMF_DISABLE)
    esp_wifi_disable_pmf_config(WIFI_IF_STA);
#endif
#endif
}
