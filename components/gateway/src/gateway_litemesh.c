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

#include <string.h>
#include <stdlib.h>
#include "esp_log.h"
#include "esp_wifi.h"

#include "esp_netif_ip_addr.h"

#include "esp_wifi_types.h"
#include "esp_gateway_config.h"
#include "esp_gateway_internal.h"
#include "esp_gateway_litemesh.h"

#define VENDOR_OUI_0                                    CONFIG_VENDOR_OUI_0
#define VENDOR_OUI_1                                    CONFIG_VENDOR_OUI_1
#define VENDOR_OUI_2                                    CONFIG_VENDOR_OUI_2

#define SINGLE_CHANNEL_SCAN_TIMES                       (1)

#define VENDOR_IE_DATA_LENGTH_MINIMUM                   (4)
#define VENDOR_IE_DATA_LENGTH                           (VENDOR_IE_DATA_LENGTH_MINIMUM + VENDOR_IE_MAX)

#define LITEMESH_VERSION                                (1)
#define LITEMESH_MAX_LEVEL                              (10)
#define LITEMESH_MAX_CONNECT_NUMBER                     CONFIG_LITEMESH_MAX_CONNECT_NUMBER
#define LITEMESH_MAX_ROUTER_NUMBER                      CONFIG_LITEMESH_MAX_ROUTER_NUMBER
#define LITEMESH_MAX_INHERITED_NETIF_NUMBER             CONFIG_LITEMESH_MAX_INHERITED_NETIF_NUMBER

#define VENDOR_IE_BASE_INFO_OFFSET                      (3)
#define LITEMESH_ROUTER_SSID_MAX_LEN                    (ESP_GATEWAY_SSID_MAX_LEN)
#define LITEMESH_ROUTER_SSID_LEN_OFFSET                 (VENDOR_IE_BASE_INFO_OFFSET)
#define LITEMESH_ROUTER_NUMBER_OFFSET                   (LITEMESH_ROUTER_SSID_LEN_OFFSET + 1)
#define LITEMESH_ROUTER_SSID_OFFSET                     (LITEMESH_ROUTER_NUMBER_OFFSET + 1)
#define LITEMESH_ROUTER_IP_SEGMENT_OFFSET               (LITEMESH_ROUTER_SSID_OFFSET + LITEMESH_ROUTER_SSID_MAX_LEN)
#define LITEMESH_EXTERNAL_NETIF_IP_SEGMENT_OFFSET       (LITEMESH_ROUTER_IP_SEGMENT_OFFSET + LITEMESH_MAX_ROUTER_NUMBER)

typedef enum {
    WIFI_ROUTER_LEVEL_0 = 0,
    WIFI_ROUTER_LEVEL_1,
    WIFI_ROUTER_LEVEL_2,
    WIFI_ROUTER_LEVEL_3,
    WIFI_ROUTER_LEVEL_4,
    WIFI_ROUTER_LEVEL_5,
    WIFI_ROUTER_LEVEL_6,
} esp_gateway_wifi_router_level_t;

typedef enum {
    VERSION = 0                        ,
    CONNECT_NUMBER_INFORMATION         ,/* 4bit(Max connect number) + 4bit(Connected station number) */
    NODE_INFORMATION                   ,/* 1bit(Connect router status) + 3bit(Reserved) + 4bit(Level) */
    ROUTER_SSID_LEN                    ,
    TRACE_ROUTER_NUMBER                ,
    ROUTER_SSID                        ,
    TRACE_ROUTER_LIST                  ,
    TRACE_EXTERNAL_NETIF_ROUTER_LIST   ,
    VENDOR_IE_MAX = LITEMESH_EXTERNAL_NETIF_IP_SEGMENT_OFFSET + LITEMESH_MAX_INHERITED_NETIF_NUMBER + ESP_GATEWAY_EXTERNAL_NETIF_MAX,
} esp_gateway_vendor_ie_t;

typedef struct {
    uint8_t version;
    uint8_t max_connection:4;
    uint8_t connected_station_number:4;
    uint8_t connect_router_status:1;
    uint8_t reserved1:3;
    uint8_t level:4;
    uint8_t router_mac[ESP_GATEWAY_MAC_MAX_LEN];
    uint8_t router_ssid_len;
    uint8_t router_number:4;
    uint8_t inherited_netif_number:4;
    uint8_t router_ssid[LITEMESH_ROUTER_SSID_MAX_LEN];
    uint8_t router_net_segment[LITEMESH_MAX_ROUTER_NUMBER];
    uint8_t inherited_net_segment[LITEMESH_MAX_INHERITED_NETIF_NUMBER];
    uint8_t self_net_segment_num:4;
    uint8_t reserved2:4;
    uint8_t self_net_segment[ESP_GATEWAY_EXTERNAL_NETIF_MAX];
} esp_gateway_litemesh_info_t;

typedef struct{
    bool valid;
    int8_t rssi;
    uint8_t level;
    uint8_t bssid[8];
} ap_info_t;

static const char *TAG = "vendor_ie";

static vendor_ie_data_t *esp_gateway_vendor_ie = NULL;
static esp_gateway_litemesh_info_t *broadcast_info = NULL;
static ap_info_t best_ap_info;

static bool connected_ap = false;
static volatile bool litemesh_scan_status = false;

extern wifi_sta_config_t router_config;

static bool esp_litemesh_info_inherit(vendor_ie_data_t *vendor_ie, esp_gateway_litemesh_info_t* out)
{
    uint8_t offset = LITEMESH_ROUTER_SSID_OFFSET;
    uint8_t router_ssid_length = vendor_ie->payload[ROUTER_SSID_LEN];
    uint8_t router_number = vendor_ie->payload[TRACE_ROUTER_NUMBER] >> 4;
    uint8_t inherited_netif_number = vendor_ie->payload[TRACE_ROUTER_NUMBER] & 0x0F;
    bool update = false;

    assert(out->version == vendor_ie->payload[VERSION]);

    if (out->connect_router_status != vendor_ie->payload[NODE_INFORMATION] >> 7) {
        out->connect_router_status = vendor_ie->payload[NODE_INFORMATION] >> 7;
        update = true;
    }

    if (out->level != ((vendor_ie->payload[NODE_INFORMATION] & 0x0F) + 1)) {
        out->level = (vendor_ie->payload[NODE_INFORMATION] & 0x0F) + 1;
        update = true;
    }

    if (out->router_ssid_len != router_ssid_length) {
        out->router_ssid_len = router_ssid_length;
        memcpy(out->router_ssid, vendor_ie->payload + offset, router_ssid_length);
        update = true;
    } else {
        if (!memcmp(out->router_ssid, vendor_ie->payload + offset, router_ssid_length)) {
            memcpy(out->router_ssid, vendor_ie->payload + offset, router_ssid_length);
            update = true;
        }
    }
    offset += router_ssid_length;

    if (out->router_number != router_number) {
        out->router_number = router_number;
        memcpy(out->router_net_segment, vendor_ie->payload + offset, router_number);
        update = true;
    } else {
        if (!memcmp(out->router_net_segment, vendor_ie->payload + offset, router_number)) {
            memcpy(out->router_net_segment, vendor_ie->payload + offset, router_number);
            update = true;
        }
    }
    offset += router_number;

    if (out->inherited_netif_number != inherited_netif_number) {
        out->inherited_netif_number = inherited_netif_number;
        memcpy(out->inherited_net_segment, vendor_ie->payload + offset, inherited_netif_number);
        update = true;
    } else {
        if (!memcmp(out->inherited_net_segment, vendor_ie->payload + offset, inherited_netif_number)) {
            memcpy(out->inherited_net_segment, vendor_ie->payload + offset, inherited_netif_number);
            update = true;
        }
    }

    return update;
}

static void esp_litemesh_info_update(esp_gateway_litemesh_info_t *current_node_info)
{
    ESP_ERROR_CHECK(esp_wifi_set_vendor_ie(false, WIFI_VND_IE_TYPE_BEACON, WIFI_VND_IE_ID_0, esp_gateway_vendor_ie));

    uint8_t offset = LITEMESH_ROUTER_SSID_OFFSET;
    uint8_t router_ssid_length = current_node_info->router_ssid_len;
    uint8_t router_number = current_node_info->router_number;

    esp_gateway_vendor_ie->payload[VERSION] = current_node_info->version;
    esp_gateway_vendor_ie->payload[CONNECT_NUMBER_INFORMATION] = ((current_node_info->max_connection << 4) | 0x0F) & (current_node_info->connected_station_number | 0xF0);
    esp_gateway_vendor_ie->payload[NODE_INFORMATION] = ((current_node_info->connect_router_status << 7) | 0x0F) & (current_node_info->level | 0x80); 
    esp_gateway_vendor_ie->payload[ROUTER_SSID_LEN] = router_ssid_length;
    esp_gateway_vendor_ie->payload[TRACE_ROUTER_NUMBER] = ((current_node_info->router_number << 4) | 0x0F) & ((current_node_info->inherited_netif_number + current_node_info->self_net_segment_num) | 0xF0);

    if (router_ssid_length) {
        memcpy(esp_gateway_vendor_ie->payload + offset, current_node_info->router_ssid, router_ssid_length);
        offset += router_ssid_length;
    }

    memcpy(esp_gateway_vendor_ie->payload + offset, current_node_info->router_net_segment, router_number);
    offset += router_number;

    memcpy(esp_gateway_vendor_ie->payload + offset, current_node_info->inherited_net_segment, current_node_info->inherited_netif_number);
    offset += current_node_info->inherited_netif_number;

    memcpy(esp_gateway_vendor_ie->payload + offset, current_node_info->self_net_segment, current_node_info->self_net_segment_num);
    offset += current_node_info->self_net_segment_num;

    esp_gateway_vendor_ie->length = VENDOR_IE_DATA_LENGTH_MINIMUM + offset;

    ESP_ERROR_CHECK(esp_wifi_set_vendor_ie(true, WIFI_VND_IE_TYPE_BEACON, WIFI_VND_IE_ID_0, esp_gateway_vendor_ie));
}

static uint8_t esp_litemesh_get_level(void)
{
    return broadcast_info->level;
}

bool esp_litemesh_network_segment_is_used(uint32_t ip)
{
    esp_ip4_addr_t ip4_addr = {.addr = ip};
    uint8_t addr3 = esp_ip4_addr3_16(&ip4_addr);

    for (uint32_t loop = 0; loop < broadcast_info->router_number; loop++) {
        if (addr3 == broadcast_info->router_net_segment[loop]) {
            return true;
        }
    }

    for (uint32_t loop = 0; loop < broadcast_info->inherited_netif_number; loop++) {
        if (addr3 == broadcast_info->inherited_net_segment[loop]) {
            return true;
        }
    }

    for (uint32_t loop = 0; loop < broadcast_info->self_net_segment_num; loop++) {
        if (addr3 == broadcast_info->self_net_segment[loop]) {
            return true;
        }
    }

    return false;
}

static void esp_litemesh_set_connected_station_number(uint8_t connected_station_number)
{
    broadcast_info->connected_station_number = connected_station_number;
}

static void esp_litemesh_set_connect_status(uint8_t connect_status)
{
    broadcast_info->connect_router_status = connect_status;
}

static void esp_litemesh_set_level(uint8_t level)
{
    broadcast_info->level = level;
}

static void esp_gateway_vendor_ie_cb(void *ctx, wifi_vendor_ie_type_t type, const uint8_t sa[6], const vendor_ie_data_t *vnd_ie, int rssi)
{
    if (type == WIFI_VND_IE_TYPE_BEACON) {
        vendor_ie_data_t *vendor_ie = (vendor_ie_data_t *)vnd_ie;
        if (vendor_ie->vendor_oui[0] == VENDOR_OUI_0 
            && vendor_ie->vendor_oui[1] == VENDOR_OUI_1 
            && vendor_ie->vendor_oui[2] == VENDOR_OUI_2) {
            if (connected_ap) { /* update parent info */
                wifi_ap_record_t ap_info;
                esp_wifi_sta_get_ap_info(&ap_info);
                if (!memcmp(ap_info.bssid , sa, sizeof(ap_info.bssid))) {
                    if (esp_litemesh_info_inherit((vendor_ie_data_t *)vendor_ie, broadcast_info)) {
                        esp_litemesh_info_update(broadcast_info);
                    }
                }
            } else {
                esp_gateway_litemesh_info_t temp;
                memset(&temp, 0x0, sizeof(esp_gateway_litemesh_info_t));
                temp.version = 1;
                temp.connected_station_number = vendor_ie->payload[CONNECT_NUMBER_INFORMATION] & 0x0F;
                temp.max_connection = vendor_ie->payload[CONNECT_NUMBER_INFORMATION] >> 4;
                esp_litemesh_info_inherit((vendor_ie_data_t *)vendor_ie, &temp);
                /* should choose the best one */
                if ((temp.max_connection > temp.connected_station_number) 
                    && (temp.connect_router_status == 1)
                    && (temp.router_ssid_len == ((strlen((char*)router_config.ssid)) > sizeof(router_config.ssid)?sizeof(router_config.ssid):strlen((char*)router_config.ssid)))
                    && !strncmp((char*)temp.router_ssid, (char*)router_config.ssid, temp.router_ssid_len)) {
                    if (((rssi > best_ap_info.rssi ) && (temp.level < best_ap_info.level))
                        || ((rssi < best_ap_info.rssi) && (rssi > best_ap_info.rssi - 15) && (temp.level < best_ap_info.level))
                        || ((rssi > best_ap_info.rssi + 15) && (temp.level > best_ap_info.level))) {
                        best_ap_info.rssi = rssi;
                        best_ap_info.valid = true;
                        memcpy(best_ap_info.bssid, sa, sizeof(best_ap_info.bssid));
                        esp_litemesh_info_inherit((vendor_ie_data_t *)vendor_ie, broadcast_info);
                    }
                }
            }

        }
    }
    return;
}

/* Event handler for catching system events */
static void esp_litemesh_event_sta_disconnected_handler(void *arg, esp_event_base_t event_base,
                          int event_id, void *event_data)
{
    uint32_t max_num = sizeof(broadcast_info->self_net_segment)/sizeof(broadcast_info->self_net_segment[0]);
    connected_ap = false;

    esp_gateway_get_external_netif_network_segment(broadcast_info->self_net_segment, &max_num);
    broadcast_info->self_net_segment_num = max_num;

    esp_litemesh_set_connect_status(0);
    esp_litemesh_info_update(broadcast_info);

    esp_gateway_wifi_set_config_into_ram(ESP_IF_WIFI_STA, &router_config);

    esp_litemesh_connect();
}

static void esp_litemesh_event_scan_done_handler(void* arg, esp_event_base_t event_base,
                                int32_t event_id, void* event_data)
{
    if (!litemesh_scan_status) {
        return;
    }
    litemesh_scan_status = false;
    ESP_LOGI(TAG, "LiteMesh Scan done\r\n");
    uint16_t count = 0;
    static uint16_t ap_channel = 0;
    static uint32_t scan_times = 0;
    ESP_ERROR_CHECK(esp_wifi_scan_get_ap_num(&count));
    wifi_ap_record_t *ap_list = (wifi_ap_record_t *)malloc(sizeof(wifi_ap_record_t) * count);
    if (ap_list == NULL) {
        ESP_LOGE(TAG, "Failed to malloc buffer to print scan results");
        return;
    }

    ESP_ERROR_CHECK(esp_wifi_scan_get_ap_records(&count, ap_list));
    for (int i = 0; i < count; ++i) {
        if (!strncmp(ESP_GATEWAY_SOFTAP_SSID, (const char *)ap_list[i].ssid, strlen(ESP_GATEWAY_SOFTAP_SSID))) {
            ap_channel = ap_list[i].primary;
            ESP_LOGI(TAG, "============ Find %s ============", ESP_GATEWAY_SOFTAP_SSID);
            break;
        }
    }
    free(ap_list);

    if (scan_times < SINGLE_CHANNEL_SCAN_TIMES) {
        esp_wifi_disconnect();
        if (ap_channel) {
            wifi_scan_config_t scanconf = {
                .channel = ap_channel,
            };
            ESP_ERROR_CHECK(esp_wifi_scan_start(&scanconf, false));
            litemesh_scan_status = true;
        } else {
            ESP_ERROR_CHECK(esp_wifi_scan_start(NULL, false));
            litemesh_scan_status = true;
        }
        scan_times++;
    } else {
        scan_times = 0;
        ap_channel = 0;

        if (best_ap_info.valid) {
            wifi_config_t wifi_cfg;

            memset(&wifi_cfg, 0x0, sizeof(wifi_cfg));
#if CONFIG_ESP_GATEWAY_SOFTAP_SSID_END_WITH_THE_MAC
            snprintf((char*)wifi_cfg.sta.ssid, sizeof(wifi_cfg.sta.ssid), "%s_%02x%02x%02x", ESP_GATEWAY_SOFTAP_SSID, best_ap_info.bssid[3], best_ap_info.bssid[4], best_ap_info.bssid[5]);
#else
            snprintf((char*)wifi_cfg.sta.ssid, sizeof(wifi_cfg.sta.ssid), "%s\r\n", ESP_GATEWAY_SOFTAP_SSID);
#endif
            strlcpy((char *)wifi_cfg.sta.password, ESP_GATEWAY_SOFTAP_PASSWORD, sizeof(wifi_cfg.sta.password));
            memcpy(wifi_cfg.sta.bssid, best_ap_info.bssid, sizeof(wifi_cfg.sta.bssid));
            wifi_cfg.sta.bssid_set = 1;
            best_ap_info.rssi = -127;
            best_ap_info.level = LITEMESH_MAX_LEVEL;
            esp_gateway_wifi_set_config_into_ram(ESP_IF_WIFI_STA, &wifi_cfg);
        }
        best_ap_info.valid = false;
        esp_wifi_connect();
    }
}

static void esp_litemesh_event_sta_got_ip_handler(void *arg, esp_event_base_t event_base,
                          int event_id, void *event_data)
{
    ip_event_got_ip_t *event = (ip_event_got_ip_t *) event_data;

    connected_ap = true;

    if (broadcast_info->level == WIFI_ROUTER_LEVEL_0) {
        broadcast_info->router_net_segment[broadcast_info->router_number++] = esp_ip4_addr3_16(&event->ip_info.ip);
        broadcast_info->level = WIFI_ROUTER_LEVEL_1;
    } else {
        uint32_t max_num = sizeof(broadcast_info->self_net_segment)/sizeof(broadcast_info->self_net_segment[0]);
        esp_gateway_get_external_netif_network_segment(broadcast_info->self_net_segment, &max_num);
        broadcast_info->self_net_segment_num = max_num;
        esp_litemesh_set_level(esp_litemesh_get_level() + 1);
    }

    esp_litemesh_set_connect_status(1);

    esp_litemesh_info_update(broadcast_info);
}

static void esp_litemesh_event_ap_staconnected_handler(void *arg, esp_event_base_t event_base,
                          int event_id, void *event_data)
{
    esp_litemesh_set_connected_station_number(broadcast_info->connected_station_number + 1);
    esp_litemesh_info_update(broadcast_info);
}

static void esp_litemesh_event_ap_stadisconnected_handler(void *arg, esp_event_base_t event_base,
                          int event_id, void *event_data)
{
    esp_litemesh_set_connected_station_number(broadcast_info->connected_station_number - 1);
    esp_litemesh_info_update(broadcast_info);
}

void esp_litemesh_connect(void)
{
    esp_wifi_scan_start(NULL, false);
    litemesh_scan_status = true;
}

esp_err_t esp_litemesh_init(void)
{
    best_ap_info.rssi = -127;
    best_ap_info.level = LITEMESH_MAX_LEVEL;

    esp_gateway_vendor_ie = malloc(sizeof(vendor_ie_data_t) + (VENDOR_IE_DATA_LENGTH * sizeof(uint8_t)));
    broadcast_info = (esp_gateway_litemesh_info_t*)malloc(sizeof(esp_gateway_litemesh_info_t));
    memset(broadcast_info, 0, sizeof(*broadcast_info));
    broadcast_info->version = LITEMESH_VERSION;
    broadcast_info->max_connection = LITEMESH_MAX_CONNECT_NUMBER;
    if (strlen((const char*)router_config.ssid) > sizeof(router_config.ssid)) {
        broadcast_info->router_ssid_len = sizeof(router_config.ssid);
    } else {
        broadcast_info->router_ssid_len = strlen((const char*)router_config.ssid);
    }
    
    memcpy(broadcast_info->router_ssid, router_config.ssid, broadcast_info->router_ssid_len);

    memset(esp_gateway_vendor_ie, 0, sizeof(*esp_gateway_vendor_ie));
    esp_gateway_vendor_ie->element_id = WIFI_VENDOR_IE_ELEMENT_ID;
    esp_gateway_vendor_ie->length = VENDOR_IE_DATA_LENGTH;
    esp_gateway_vendor_ie->vendor_oui[0] = VENDOR_OUI_0;
    esp_gateway_vendor_ie->vendor_oui[1] = VENDOR_OUI_1;
    esp_gateway_vendor_ie->vendor_oui[2] = VENDOR_OUI_2;

    ESP_ERROR_CHECK(esp_wifi_set_vendor_ie(true, WIFI_VND_IE_TYPE_BEACON, WIFI_VND_IE_ID_0, esp_gateway_vendor_ie));
    ESP_ERROR_CHECK(esp_wifi_set_vendor_ie_cb((esp_vendor_ie_cb_t)esp_gateway_vendor_ie_cb, NULL));

    /* Register our event handler for Wi-Fi, IP and Provisioning related events */
    ESP_ERROR_CHECK(esp_event_handler_instance_register(WIFI_EVENT, WIFI_EVENT_STA_DISCONNECTED, esp_litemesh_event_sta_disconnected_handler, NULL, NULL));
    ESP_ERROR_CHECK(esp_event_handler_instance_register(WIFI_EVENT, WIFI_EVENT_SCAN_DONE, esp_litemesh_event_scan_done_handler, NULL, NULL));
    ESP_ERROR_CHECK(esp_event_handler_instance_register(IP_EVENT, IP_EVENT_STA_GOT_IP, esp_litemesh_event_sta_got_ip_handler, NULL, NULL));

    /* Register our event handler for Wi-Fi, IP and Provisioning related events */
    ESP_ERROR_CHECK(esp_event_handler_instance_register(WIFI_EVENT, WIFI_EVENT_AP_STACONNECTED, esp_litemesh_event_ap_staconnected_handler, NULL, NULL));
    ESP_ERROR_CHECK(esp_event_handler_instance_register(WIFI_EVENT, WIFI_EVENT_AP_STADISCONNECTED, esp_litemesh_event_ap_stadisconnected_handler, NULL, NULL));

    esp_litemesh_connect();
    ESP_LOGI(TAG, "Litemesh Scan start\r\n");
    esp_litemesh_info_update(broadcast_info);

    return ESP_OK;
}
