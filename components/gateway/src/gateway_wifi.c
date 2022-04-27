// Copyright 2021-2022 Espressif Systems (Shanghai) PTE LTD
//
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
//
//     http://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.

#include <stdio.h>
#include <string.h>
#include "esp_log.h"
#include "esp_wifi.h"
#include "esp_system.h"
#include "driver/uart.h"
#include "freertos/event_groups.h"

#include "lwip/opt.h"
#include "lwip/err.h"
#include "lwip/sys.h"
#include "lwip/ip_addr.h"
#include "lwip/lwip_napt.h"
#include "dhcpserver/dhcpserver.h"

#include "esp_gateway_config.h"
#include "esp_gateway_wifi.h"
#include "esp_gateway_internal.h"

#if defined(CONFIG_GATEWAY_EXTERNAL_NETIF_STATION) || defined(CONFIG_GATEWAY_DATA_FORWARDING_NETIF_SOFTAP)

#define GATEWAY_EVENT_STA_CONNECTED  BIT0

static EventGroupHandle_t s_wifi_event_group = NULL;
static const char *TAG = "gateway_wifi";

#if CONFIG_LITEMESH_ENABLE
#include "esp_litemesh.h"
#endif

static esp_err_t esp_gateway_wifi_set(wifi_mode_t mode, const char *ssid, const char *password, const char *bssid)
{
    ESP_PARAM_CHECK(ssid);
    ESP_PARAM_CHECK(password);

    wifi_config_t wifi_cfg = {0};

    if (mode & WIFI_MODE_STA) {
        strncpy((char *)wifi_cfg.sta.ssid, ssid, sizeof(wifi_cfg.sta.ssid));
        strlcpy((char *)wifi_cfg.sta.password, password, sizeof(wifi_cfg.sta.password));
        if (bssid != NULL) {
            wifi_cfg.sta.bssid_set = 1;
            memcpy((char *)wifi_cfg.sta.bssid, bssid, sizeof(wifi_cfg.sta.bssid));
            ESP_LOGI(TAG, "sta ssid: %s password: %s MAC "MACSTR"", ssid, password, MAC2STR(wifi_cfg.sta.bssid));
        } else {
            ESP_LOGI(TAG, "sta ssid: %s password: %s", ssid, password);
        }

        ESP_ERROR_CHECK(esp_wifi_set_config(ESP_IF_WIFI_STA, &wifi_cfg));
    }

    if (mode & WIFI_MODE_AP) {
        wifi_cfg.ap.max_connection = 10;
        wifi_cfg.ap.authmode = strlen(password) < 8 ? WIFI_AUTH_OPEN : WIFI_AUTH_WPA2_PSK;
        strncpy((char *)wifi_cfg.ap.ssid, ssid, sizeof(wifi_cfg.ap.ssid));
        strlcpy((char *)wifi_cfg.ap.password, password, sizeof(wifi_cfg.ap.password));

        ESP_ERROR_CHECK(esp_wifi_set_config(ESP_IF_WIFI_AP, &wifi_cfg));

        ESP_LOGI(TAG, "softap ssid: %s password: %s", ssid, password);
    }

    return ESP_OK;
}

/* Event handler for catching system events */
static void wifi_event_sta_disconnected_handler(void *arg, esp_event_base_t event_base,
                                                int32_t event_id, void *event_data)
{
#if !CONFIG_LITEMESH_ENABLE
    ESP_LOGE(TAG, "Disconnected. Connecting to the AP again...");
    esp_wifi_connect();
#endif
    xEventGroupClearBits(s_wifi_event_group, GATEWAY_EVENT_STA_CONNECTED);
}

static void ip_event_sta_got_ip_handler(void *arg, esp_event_base_t event_base,
                                        int32_t event_id, void *event_data)
{
    ip_event_got_ip_t *event = (ip_event_got_ip_t *) event_data;
    ESP_LOGI(TAG, "Connected with IP Address:" IPSTR, IP2STR(&event->ip_info.ip));

    esp_gateway_netif_network_segment_conflict_update(event->esp_netif);
    xEventGroupSetBits(s_wifi_event_group, GATEWAY_EVENT_STA_CONNECTED);
}

static void wifi_event_ap_staconnected_handler(void *arg, esp_event_base_t event_base,
                                               int32_t event_id, void *event_data)
{
    ESP_LOGI(TAG, "STA Connecting to the AP again...");
}

static void wifi_event_ap_stadisconnected_handler(void *arg, esp_event_base_t event_base,
                                                  int32_t event_id, void *event_data)
{
    ESP_LOGE(TAG, "STA Disconnect to the AP");
}

static esp_err_t esp_gateway_wifi_init(void)
{
    if (s_wifi_event_group) {
        return ESP_OK;
    }

    s_wifi_event_group = xEventGroupCreate();

    wifi_init_config_t cfg = WIFI_INIT_CONFIG_DEFAULT();
    ESP_ERROR_CHECK(esp_wifi_init(&cfg));

    ESP_ERROR_CHECK(esp_wifi_set_storage(WIFI_STORAGE_RAM));
    ESP_ERROR_CHECK(esp_wifi_set_mode(WIFI_MODE_NULL));

    esp_wifi_start();

    return ESP_OK;
}
#endif /* CONFIG_GATEWAY_EXTERNAL_NETIF_STATION || CONFIG_GATEWAY_DATA_FORWARDING_NETIF_SOFTAP */

#if defined(CONFIG_GATEWAY_EXTERNAL_NETIF_STATION)
#ifdef CONFIG_LITEMESH_ENABLE
static void esp_litemesh_event_ip_changed_handler(void *arg, esp_event_base_t event_base,
                                                          int32_t event_id, void *event_data)
{
    switch(event_id) {
        case LITEMESH_EVENT_STARTED:
            ESP_LOGI(TAG, "LiteMesh connecting");
            esp_litemesh_connect();
            break;
        case LITEMESH_EVENT_INHERITED_NET_SEGMENT_CHANGED:
            ESP_LOGI(TAG, "netif network segment conflict check");
            esp_gateway_netif_network_segment_conflict_update(NULL);
            break;
        case LITEMESH_EVENT_ROUTER_INFO_CHANGED:
            break;
    }
}
#endif

esp_netif_t* esp_gateway_create_station_netif(esp_netif_ip_info_t* ip_info, uint8_t mac[6], bool data_forwarding, bool enable_dhcps)
{
    esp_netif_t *wifi_netif = NULL;
    wifi_mode_t mode = WIFI_MODE_NULL;

    if (data_forwarding || enable_dhcps) {
        return wifi_netif;
    }

    esp_gateway_wifi_init();
    wifi_netif = esp_netif_create_default_wifi_sta();
    esp_gateway_netif_list_add(wifi_netif, NULL);

    esp_wifi_get_mode(&mode);
    mode |= WIFI_MODE_STA;
    ESP_ERROR_CHECK(esp_wifi_set_mode(mode));

#if !CONFIG_LITEMESH_ENABLE
    wifi_sta_config_t router_config;
    /* Get WiFi Station configuration */
    esp_wifi_get_config(WIFI_IF_STA, (wifi_config_t*)&router_config);

    /* Get Wi-Fi Station ssid success */
    if (strlen((const char*)router_config.ssid)) {
        ESP_LOGI(TAG, "Found ssid %s",     (const char*) router_config.ssid);
        ESP_LOGI(TAG, "Found password %s", (const char*) router_config.password);
        esp_wifi_connect();
    }
#endif /* CONFIG_LITEMESH_ENABLE */

    /* Register our event handler for Wi-Fi, IP and Provisioning related events */
    ESP_ERROR_CHECK(esp_event_handler_instance_register(WIFI_EVENT, WIFI_EVENT_STA_DISCONNECTED, &wifi_event_sta_disconnected_handler, NULL, NULL));
    ESP_ERROR_CHECK(esp_event_handler_instance_register(IP_EVENT, IP_EVENT_STA_GOT_IP, &ip_event_sta_got_ip_handler, NULL, NULL));

    return wifi_netif;
}
#endif /* CONFIG_GATEWAY_EXTERNAL_NETIF_STATION */

#if defined(CONFIG_GATEWAY_DATA_FORWARDING_NETIF_SOFTAP)
static esp_err_t softap_netif_dhcp_status_change_cb(esp_ip_addr_t *ip_info)
{
    esp_err_t ret = ESP_FAIL;

    if (esp_wifi_deauth_sta(0) == ESP_OK) {
        ret = ESP_OK;
    }

    return ret;
}

esp_netif_t* esp_gateway_create_softap_netif(esp_netif_ip_info_t* ip_info, uint8_t mac[6], bool data_forwarding, bool enable_dhcps)
{
    esp_netif_ip_info_t netif_ip;
    esp_netif_ip_info_t allocate_ip_info;
    char softap_ssid[ESP_GATEWAY_SSID_MAX_LEN];
    esp_netif_t *wifi_netif = NULL;
    wifi_mode_t mode = WIFI_MODE_NULL;

    if (!data_forwarding) {
        return wifi_netif;
    }

    esp_gateway_wifi_init();

    wifi_netif = esp_netif_create_default_wifi_ap();

    ESP_ERROR_CHECK(esp_netif_dhcps_stop(wifi_netif));
    if (ip_info) {
        ESP_ERROR_CHECK(esp_netif_set_ip_info(wifi_netif, ip_info));
    } else {
        if (enable_dhcps) {
            esp_gateway_netif_request_ip(&allocate_ip_info);
            esp_netif_set_ip_info(wifi_netif, &allocate_ip_info);
        }
    }
    esp_netif_get_ip_info(wifi_netif, &netif_ip);
    ESP_LOGI(TAG, "IP Address:" IPSTR, IP2STR(&netif_ip.ip));
    esp_gateway_netif_list_add(wifi_netif, softap_netif_dhcp_status_change_cb);

    if (enable_dhcps) {
        esp_netif_dhcps_start(wifi_netif);
        esp_netif_dns_info_t dns;
        dns.ip.u_addr.ip4.addr = ESP_IP4TOADDR(114, 114, 114, 114);
        dns.ip.type = IPADDR_TYPE_V4;
        dhcps_offer_t dhcps_dns_value = OFFER_DNS;
        esp_netif_dhcps_option(wifi_netif, ESP_NETIF_OP_SET, ESP_NETIF_DOMAIN_NAME_SERVER, &dhcps_dns_value, sizeof(dhcps_dns_value));
        esp_netif_set_dns_info(wifi_netif, ESP_NETIF_DNS_MAIN, &dns);
    }

    /* Register our event handler for Wi-Fi, IP and Provisioning related events */
    ESP_ERROR_CHECK(esp_event_handler_instance_register(WIFI_EVENT, WIFI_EVENT_AP_STACONNECTED, &wifi_event_ap_staconnected_handler, NULL, NULL));
    ESP_ERROR_CHECK(esp_event_handler_instance_register(WIFI_EVENT, WIFI_EVENT_AP_STADISCONNECTED, &wifi_event_ap_stadisconnected_handler, NULL, NULL));
#ifdef CONFIG_LITEMESH_ENABLE
    ESP_ERROR_CHECK(esp_event_handler_instance_register(LITEMESH_EVENT, ESP_EVENT_ANY_ID, &esp_litemesh_event_ip_changed_handler, NULL, NULL));
#endif
    esp_wifi_get_mode(&mode);
    mode |= WIFI_MODE_AP;
    ESP_ERROR_CHECK(esp_wifi_set_mode(mode));

#if CONFIG_ESP_GATEWAY_SOFTAP_SSID_END_WITH_THE_MAC
    uint8_t softap_mac[ESP_GATEWAY_MAC_MAX_LEN];
    esp_wifi_get_mac(WIFI_IF_AP, softap_mac);
    snprintf(softap_ssid, sizeof(softap_ssid), "%s_%02x%02x%02x", ESP_GATEWAY_SOFTAP_SSID, softap_mac[3], softap_mac[4], softap_mac[5]);
#else
    snprintf(softap_ssid, sizeof(softap_ssid), "%s", ESP_GATEWAY_SOFTAP_SSID);
#endif
    esp_gateway_wifi_set(WIFI_MODE_AP, softap_ssid, ESP_GATEWAY_SOFTAP_PASSWORD, NULL);
    ip_napt_enable(netif_ip.ip.addr, 1);

    return wifi_netif;
}
#endif // CONFIG_GATEWAY_DATA_FORWARDING_NETIF_SOFTAP