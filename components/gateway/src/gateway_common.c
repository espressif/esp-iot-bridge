// Copyright 2015-2020 Espressif Systems (Shanghai) PTE LTD
//
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at

//     http://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.
//
#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <string.h>
#include <sys/fcntl.h>
#include <sys/errno.h>
#include <sys/unistd.h>
#include <sys/select.h>

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

#include "esp_log.h"
#include "esp_netif.h"
#include "esp_system.h"

#include "esp_gateway_internal.h"
#include "esp_gateway.h"

typedef struct gateway_netif {
    esp_netif_t* netif;
    struct gateway_netif* next;
} gateway_netif_t;

static const char* TAG = "gateway_common";
static gateway_netif_t* gateway_link = NULL;

esp_err_t esp_geteway_netif_list_add(esp_netif_t* netif)
{
    gateway_netif_t* new = gateway_link;
    gateway_netif_t* tail = NULL;

    while (new) {
        if (new->netif == netif) {
            return ESP_OK;
        }
        tail = new;
        new = new->next;
    }

    // not found, create a new
    new = (gateway_netif_t*)malloc(sizeof(gateway_netif_t));
    if (new == NULL) {
        ESP_LOGE(TAG, "add fail");
    }
    new->netif = netif;
    new->next = NULL;

    if (tail == NULL) { // the first one
        gateway_link = new;
    } else {
        tail->next = new;
    }
    ESP_LOGI(TAG, "add success");

    return ESP_FAIL;
}

esp_err_t esp_geteway_netif_list_remove(esp_netif_t* netif)
{
    gateway_netif_t* current = gateway_link;
    gateway_netif_t* prev = NULL;

    while (current) {
        if (current->netif == netif) {
            if (prev == NULL) {
                gateway_link = gateway_link->next;
            } else {
                prev->next = current->next;
            }
            break;
        }

        prev = current;
        current = current->next;
    }

    return ESP_OK;
}

static bool esp_geteway_netif_network_segment_is_used(uint32_t ip)
{
    gateway_netif_t* p = gateway_link;
    esp_netif_ip_info_t netif_ip = { 0 };
    while (p) {
        esp_netif_get_ip_info(p->netif, &netif_ip);
        if (esp_ip4_addr3_16((esp_ip4_addr_t*)&ip) == esp_ip4_addr3_16(&netif_ip.ip)) {
            return true;
        }
        
        p = p->next;
    }

    return false;
}

esp_err_t esp_geteway_netif_request_ip(esp_netif_ip_info_t* ip_info)
{
    uint8_t gateway_ip = 4;

    for (gateway_ip = 4; gateway_ip < 255; gateway_ip++) {
        if(!esp_geteway_netif_network_segment_is_used(ESP_IP4TOADDR(192, 168, gateway_ip, 1))) {
            ip_info->ip.addr = ESP_IP4TOADDR(192, 168, gateway_ip, 1);
            ip_info->gw.addr = ESP_IP4TOADDR(192, 168, gateway_ip, 1);
            ip_info->netmask.addr = ESP_IP4TOADDR(255, 255, 255, 0);
            ESP_LOGI("ip select", "IP Address:" IPSTR, IP2STR(&ip_info->ip));
            ESP_LOGI("ip select", "GW Address:" IPSTR, IP2STR(&ip_info->gw));
            ESP_LOGI("ip select", "NM Address:" IPSTR, IP2STR(&ip_info->netmask));

            return ESP_OK;
        }
        gateway_ip++;
    }

    return ESP_FAIL;
}

static bool esp_geteway_netif_mac_is_used(uint8_t mac[6])
{
    gateway_netif_t* p = gateway_link;
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

esp_err_t esp_geteway_netif_request_mac(uint8_t* mac)
{
    uint8_t netif_mac[6] = { 0 };
    // esp_read_mac(netif_mac, ESP_MAC_ETH);
    esp_base_mac_addr_get(netif_mac);

    while (1) {
        if (!esp_geteway_netif_mac_is_used(netif_mac)){
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

esp_netif_t* esp_gateway_create_netif(esp_netif_config_t* config, esp_netif_ip_info_t* custom_ip_info, uint8_t custom_mac[6], bool enable_dhcps)
{
    esp_netif_ip_info_t allocate_ip_info = { 0 };
    uint8_t allocate_mac[6] = { 0 };
    esp_netif_t* netif = esp_netif_new(config);
    assert(netif);

    esp_netif_dhcps_stop(netif);
    if (custom_ip_info) { // Custom IP
        esp_netif_set_ip_info(netif, custom_ip_info);
    } else {
        esp_geteway_netif_request_ip(&allocate_ip_info);
        esp_netif_set_ip_info(netif, &allocate_ip_info);
    }

    if (custom_mac) { // Custom MAC
        ESP_ERROR_CHECK(esp_netif_set_mac(netif, custom_mac));
    } else {
        esp_geteway_netif_request_mac(allocate_mac);
        esp_netif_set_mac(netif, allocate_mac);
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

void esp_gateway_create_all_netif(void)
{
#if defined(CONFIG_GATEWAY_EXTERNAL_NETIF_MODEM)
    esp_gateway_create_modem_netif(NULL, NULL, false, false);
#endif

#if defined(CONFIG_GATEWAY_DATA_FORWARDING_NETIF_SOFTAP)
    esp_gateway_create_softap_netif(NULL, NULL, true, true);
#endif

#if defined(CONFIG_GATEWAY_EXTERNAL_NETIF_STATION)
    esp_gateway_create_station_netif(NULL, NULL, false, false);
#endif

#if defined(CONFIG_GATEWAY_DATA_FORWARDING_NETIF_SDIO)
    esp_gateway_create_sdio_netif(NULL, NULL, true, true);
#endif

#if defined(CONFIG_GATEWAY_DATA_FORWARDING_NETIF_SPI)
    esp_gateway_create_spi_netif(NULL, NULL, true, true);
#endif
}