/*
 * SPDX-FileCopyrightText: 2021-2022 Espressif Systems (Shanghai) CO LTD
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <stdio.h>
#include <string.h>
#include "esp_log.h"
#include "esp_wifi.h"
#include "esp_system.h"
#include "freertos/event_groups.h"

#include "lwip/opt.h"
#include "lwip/err.h"
#include "lwip/sys.h"
#include "lwip/ip_addr.h"
#include "lwip/lwip_napt.h"
#include "dhcpserver/dhcpserver.h"

#include "esp_bridge_config.h"
#include "esp_bridge_wifi.h"
#include "esp_bridge_internal.h"

#include "esp_bridge.h"

#if defined(CONFIG_BRIDGE_EXTERNAL_NETIF_STATION) || defined(CONFIG_BRIDGE_DATA_FORWARDING_NETIF_SOFTAP)

#define BRIDGE_EVENT_STA_CONNECTED  BIT0

static const char *TAG = "bridge_wifi";
static EventGroupHandle_t s_wifi_event_group = NULL;

esp_err_t esp_bridge_wifi_set_config(wifi_interface_t interface, wifi_config_t *conf)
{
    esp_err_t ret = ESP_FAIL;
    switch (interface) {
    case WIFI_IF_STA:
        ESP_LOGI(TAG, "[%s] sta ssid: %s password: %s", __func__, conf->sta.ssid, conf->sta.password);
        ret = esp_wifi_set_config(WIFI_IF_STA, conf);
        break;

    case WIFI_IF_AP: {
#if CONFIG_BRIDGE_SOFTAP_SSID_END_WITH_THE_MAC
        uint8_t softap_mac[BRIDGE_MAC_MAX_LEN];
        char suffix[8];
        esp_wifi_get_mac(WIFI_IF_AP, softap_mac);
        char temp_ssid[sizeof(conf->ap.ssid) + 1];
        memcpy(temp_ssid, (char*)conf->ap.ssid, sizeof(temp_ssid));
        snprintf(suffix, sizeof(suffix), "_%02x%02x%02x", softap_mac[3], softap_mac[4], softap_mac[5]);
        if (strlen(temp_ssid) + strlen(suffix) > 32) {
            return ESP_FAIL;
        }
        strcat(temp_ssid, suffix);
        memcpy((char*)conf->ap.ssid, temp_ssid, sizeof(conf->ap.ssid));
#endif
        conf->ap.max_connection = BRIDGE_SOFTAP_MAX_CONNECT_NUMBER;
        conf->ap.authmode = strlen((char*)conf->ap.password) < 8 ? WIFI_AUTH_OPEN : WIFI_AUTH_WPA2_PSK;
        ESP_LOGI(TAG, "[%s] softap ssid: %s password: %s", __func__, conf->ap.ssid, conf->ap.password);
        ret = esp_wifi_set_config(WIFI_IF_AP, conf);

        ESP_LOGI(TAG, "SoftAP config changed, deauth all station");
        esp_wifi_deauth_sta(0);
        break;
    }

    default:
        break;
    }
    return ret;
}

esp_err_t esp_bridge_wifi_set(wifi_mode_t mode,
                              const char *ssid,
                              const char *password,
                              const char *bssid)
{
    ESP_PARAM_CHECK(ssid);
    ESP_PARAM_CHECK(password);

    wifi_config_t wifi_cfg;
    memset(&wifi_cfg, 0x0, sizeof(wifi_config_t));

    if (mode & WIFI_MODE_STA) {
        memcpy((char *)wifi_cfg.sta.ssid, ssid, sizeof(wifi_cfg.sta.ssid));
        strlcpy((char *)wifi_cfg.sta.password, password, sizeof(wifi_cfg.sta.password));

        if (bssid != NULL) {
            wifi_cfg.sta.bssid_set = 1;
            memcpy((char *)wifi_cfg.sta.bssid, bssid, sizeof(wifi_cfg.sta.bssid));
            ESP_LOGI(TAG, "[%s] sta ssid: %s password: %s MAC "MACSTR"", __func__, ssid, password, MAC2STR(wifi_cfg.sta.bssid));
        } else {
            ESP_LOGI(TAG, "[%s] sta ssid: %s password: %s", __func__, ssid, password);
        }

        ESP_ERROR_CHECK(esp_wifi_set_config(ESP_IF_WIFI_STA, &wifi_cfg));
    }

    if (mode & WIFI_MODE_AP) {
        char softap_ssid[BRIDGE_SSID_MAX_LEN + 1];
        memcpy(softap_ssid, ssid, sizeof(softap_ssid));
#if CONFIG_BRIDGE_SOFTAP_SSID_END_WITH_THE_MAC
        uint8_t softap_mac[BRIDGE_MAC_MAX_LEN];
        char suffix[8];
        esp_wifi_get_mac(WIFI_IF_AP, softap_mac);
        snprintf(suffix, sizeof(suffix), "_%02x%02x%02x", softap_mac[3], softap_mac[4], softap_mac[5]);
        strcat(softap_ssid, suffix);
#endif
        memcpy((char*)wifi_cfg.ap.ssid, softap_ssid, sizeof(wifi_cfg.ap.ssid));
        strlcpy((char*)wifi_cfg.ap.password, password, sizeof(wifi_cfg.ap.password));
        wifi_cfg.ap.max_connection = BRIDGE_SOFTAP_MAX_CONNECT_NUMBER;
        wifi_cfg.ap.authmode = strlen((char*)wifi_cfg.ap.password) < 8 ? WIFI_AUTH_OPEN : WIFI_AUTH_WPA2_PSK;
        ESP_ERROR_CHECK(esp_wifi_set_config(ESP_IF_WIFI_AP, &wifi_cfg));

        ESP_LOGI(TAG, "[%s] softap ssid: %s password: %s", __func__, (char*)wifi_cfg.ap.ssid, (char*)wifi_cfg.ap.password);
        ESP_LOGI(TAG, "SoftAP config changed, deauth all station");
        esp_wifi_deauth_sta(0);
    }

    return ESP_OK;
}

static esp_err_t esp_bridge_wifi_init(void)
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
#endif /* CONFIG_BRIDGE_EXTERNAL_NETIF_STATION || CONFIG_BRIDGE_DATA_FORWARDING_NETIF_SOFTAP */

#if defined(CONFIG_BRIDGE_EXTERNAL_NETIF_STATION)

static void ip_event_sta_got_ip_handler(void *arg, esp_event_base_t event_base,
    int32_t event_id, void *event_data)
{
    ip_event_got_ip_t *event = (ip_event_got_ip_t*)event_data;
    ESP_LOGI(TAG, "Connected with IP Address:" IPSTR, IP2STR(&event->ip_info.ip));

    esp_bridge_update_dns_info(event->esp_netif, NULL);

    esp_bridge_netif_network_segment_conflict_update(event->esp_netif);
    xEventGroupSetBits(s_wifi_event_group, BRIDGE_EVENT_STA_CONNECTED);
}

/* Event handler for catching system events */
static void wifi_event_sta_disconnected_handler(void *arg, esp_event_base_t event_base,
    int32_t event_id, void *event_data)
{
    IOT_BRIDGE_NAPT_TABLE_CLEAR();
#if !CONFIG_BRIDGE_STATION_CANCEL_AUTO_CONNECT_WHEN_DISCONNECTED
    ESP_LOGE(TAG, "Disconnected. Connecting to the AP again...");
    esp_wifi_connect();
#endif
    xEventGroupClearBits(s_wifi_event_group, BRIDGE_EVENT_STA_CONNECTED);
}

esp_netif_t* esp_bridge_create_station_netif(esp_netif_ip_info_t* ip_info, uint8_t mac[6], bool data_forwarding, bool enable_dhcps)
{
    esp_netif_t* wifi_netif = NULL;
    wifi_mode_t mode = WIFI_MODE_NULL;

    if (data_forwarding || enable_dhcps) {
        return wifi_netif;
    }

    esp_bridge_wifi_init();
    wifi_netif = esp_netif_create_default_wifi_sta();
    esp_bridge_netif_list_add(wifi_netif, NULL);

    esp_wifi_get_mode(&mode);
    if (mode != WIFI_MODE_STA && mode != WIFI_MODE_APSTA) {
        mode |= WIFI_MODE_STA;
        ESP_ERROR_CHECK(esp_wifi_set_mode(mode));
    }

    wifi_sta_config_t router_config;
    /* Get WiFi Station configuration */
    esp_wifi_get_config(WIFI_IF_STA, (wifi_config_t*)&router_config);

    /* Get Wi-Fi Station ssid success */
    if (strlen((const char*)router_config.ssid)) {
        ESP_LOGI(TAG, "Found ssid %s", (const char*)router_config.ssid);
        ESP_LOGI(TAG, "Found password %s", (const char*)router_config.password);
    }

    if (ip_info) {
        esp_bridge_netif_set_ip_info(wifi_netif, ip_info, true, true);
    } else {
        esp_netif_ip_info_t ip_info_from_nvs;
        bool conflict_check = true;
        if (esp_bridge_load_ip_info_from_nvs(esp_netif_get_ifkey(wifi_netif), &ip_info_from_nvs, &conflict_check) == ESP_OK) {
            esp_bridge_netif_set_ip_info(wifi_netif, &ip_info_from_nvs, true, conflict_check);
        }
    }

    /* Register our event handler for Wi-Fi, IP and Provisioning related events */
    ESP_ERROR_CHECK(esp_event_handler_instance_register(WIFI_EVENT, WIFI_EVENT_STA_DISCONNECTED, &wifi_event_sta_disconnected_handler, NULL, NULL));
    ESP_ERROR_CHECK(esp_event_handler_instance_register(IP_EVENT, IP_EVENT_STA_GOT_IP, &ip_event_sta_got_ip_handler, NULL, NULL));

    return wifi_netif;
}
#endif /* CONFIG_BRIDGE_EXTERNAL_NETIF_STATION */

#if defined(CONFIG_BRIDGE_DATA_FORWARDING_NETIF_SOFTAP)
static bool esp_bridge_softap_dhcps = false;

static void wifi_event_ap_start_handler(void *arg, esp_event_base_t event_base,
    int32_t event_id, void *event_data)
{
    esp_netif_t *softap_netif = esp_netif_get_handle_from_ifkey("WIFI_AP_DEF");
    if (softap_netif) {
        if (esp_bridge_softap_dhcps) {
            esp_netif_t *external_netif = NULL;
#if defined(CONFIG_BRIDGE_EXTERNAL_NETIF_SPI)
            external_netif = esp_netif_get_handle_from_ifkey("SPI_DEF");
#endif
#if defined(CONFIG_BRIDGE_EXTERNAL_NETIF_SDIO)
            external_netif = esp_netif_get_handle_from_ifkey("SDIO_DEF");
#endif
#if (defined(CONFIG_BRIDGE_EXTERNAL_NETIF_ETHERNET) || defined(CONFIG_BRIDGE_NETIF_ETHERNET_AUTO_WAN_OR_LAN))
            external_netif = esp_netif_get_handle_from_ifkey("ETH_WAN");
#endif
#if defined(CONFIG_BRIDGE_EXTERNAL_NETIF_MODEM)
            external_netif = esp_netif_get_handle_from_ifkey("PPP_DEF");
#endif
#if defined(CONFIG_BRIDGE_EXTERNAL_NETIF_STATION)
            external_netif = esp_netif_get_handle_from_ifkey("WIFI_STA_DEF");
#endif
            esp_bridge_update_dns_info(external_netif, softap_netif);
        }
        esp_netif_ip_info_t netif_ip;
        esp_netif_get_ip_info(softap_netif, &netif_ip);
#if CONFIG_LWIP_IPV4_NAPT
        ip_napt_enable(netif_ip.ip.addr, 1);
#endif
    }
}

static void wifi_event_ap_staconnected_handler(void* arg, esp_event_base_t event_base,
    int32_t event_id, void* event_data)
{
    ESP_LOGI(TAG, "STA Connecting to the AP again...");
}

static void wifi_event_ap_stadisconnected_handler(void* arg, esp_event_base_t event_base,
    int32_t event_id, void* event_data)
{
    ESP_LOGE(TAG, "STA Disconnect to the AP");
}

static esp_err_t softap_netif_dhcp_status_change_cb(esp_ip_addr_t* ip_info)
{
    esp_err_t ret = ESP_FAIL;
    ESP_LOGI(TAG, "SoftAP IP network segment has changed, deauth all station");
    if (esp_wifi_deauth_sta(0) == ESP_OK) {
        ret = ESP_OK;
    }

    return ret;
}

esp_netif_t* esp_bridge_create_softap_netif(esp_netif_ip_info_t *ip_info, uint8_t mac[6], bool data_forwarding, bool enable_dhcps)
{
    esp_netif_ip_info_t allocate_ip_info;
    esp_netif_t* wifi_netif = NULL;
    wifi_mode_t mode = WIFI_MODE_NULL;

    if (!data_forwarding) {
        return wifi_netif;
    }

    esp_bridge_wifi_init();

    wifi_netif = esp_netif_create_default_wifi_ap();

    if (enable_dhcps) {
        dhcps_offer_t dhcps_dns_value = OFFER_DNS;
        ESP_ERROR_CHECK(esp_netif_dhcps_option(wifi_netif, ESP_NETIF_OP_SET, ESP_NETIF_DOMAIN_NAME_SERVER, &dhcps_dns_value, sizeof(dhcps_dns_value)));
    }

    esp_wifi_get_mode(&mode);
    if (mode != WIFI_MODE_AP && mode != WIFI_MODE_APSTA) {
        mode |= WIFI_MODE_AP;
        ESP_ERROR_CHECK(esp_wifi_set_mode(mode));
    }

    esp_bridge_netif_list_add(wifi_netif, softap_netif_dhcp_status_change_cb);

    if (ip_info) {
        esp_bridge_netif_set_ip_info(wifi_netif, ip_info, true, true);
    } else {
        bool conflict_check = true;
        if (esp_bridge_load_ip_info_from_nvs(esp_netif_get_ifkey(wifi_netif), &allocate_ip_info, &conflict_check) != ESP_OK) {
            if (enable_dhcps) {
                esp_bridge_netif_request_ip(&allocate_ip_info);
            }
        }
        esp_bridge_netif_set_ip_info(wifi_netif, &allocate_ip_info, true, conflict_check);
    }

    esp_bridge_softap_dhcps = enable_dhcps;

    /* Register our event handler for Wi-Fi, IP and Provisioning related events */
    ESP_ERROR_CHECK(esp_event_handler_instance_register(WIFI_EVENT, WIFI_EVENT_AP_START, &wifi_event_ap_start_handler, NULL, NULL));
    ESP_ERROR_CHECK(esp_event_handler_instance_register(WIFI_EVENT, WIFI_EVENT_AP_STACONNECTED, &wifi_event_ap_staconnected_handler, NULL, NULL));
    ESP_ERROR_CHECK(esp_event_handler_instance_register(WIFI_EVENT, WIFI_EVENT_AP_STADISCONNECTED, &wifi_event_ap_stadisconnected_handler, NULL, NULL));

    return wifi_netif;
}
#endif // CONFIG_BRIDGE_DATA_FORWARDING_NETIF_SOFTAP
