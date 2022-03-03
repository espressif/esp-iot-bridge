// Copyright 2021 Espressif Systems (Shanghai) PTE LTD
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

#include "esp_gateway_vendor_ie.h"

#if SET_VENDOR_IE

static const char *TAG = "vendor_ie";

bool esp_gateway_vendor_status = false;
bool first_vendor_ie_tag = true;
ap_router_t *ap_router = NULL;
vendor_ie_data_t *esp_gateway_vendor_ie = NULL;

void esp_gateway_vendor_ie_info(vendor_ie_data_t *vendor_ie)
{
    ESP_LOGI(TAG, "vendor_ie length:%d , MAX_CONNECT_NUMBER:%d , STATION_NUMBER:%d , ROUTER_RSSI:-%-2d , CONNECT_ROUTER_STATUS:%d , LEVEL:%d ", 
        vendor_ie->length, 
        vendor_ie->payload[MAX_CONNECT_NUMBER], 
        vendor_ie->payload[STATION_NUMBER],
        vendor_ie->payload[ROUTER_RSSI],
        vendor_ie->payload[CONNECT_ROUTER_STATUS],
        vendor_ie->payload[LEVEL]);
}

void esp_gateway_vendor_ie_cb(void *ctx, wifi_vendor_ie_type_t type, const uint8_t sa[6], const vendor_ie_data_t *vnd_ie, int rssi)
{
    if (type == WIFI_VND_IE_TYPE_BEACON) {
        vendor_ie_data_t *vendor_ie;
        vendor_ie = vnd_ie;
        if (vendor_ie->vendor_oui[0] == VENDOR_OUI_0 && vendor_ie->vendor_oui[1] == VENDOR_OUI_1 && vendor_ie->vendor_oui[2] == VENDOR_OUI_2) {
            // memcpy(ap_router->router_mac, (char*)sa, MAC_LEN);
            // ESP_LOGI(TAG, "MAC "MACSTR"", MAC2STR(ap_router->router_mac));
            // esp_gateway_vendor_ie_info(vendor_ie);
            if (vendor_ie->payload[MAX_CONNECT_NUMBER] > vendor_ie->payload[STATION_NUMBER]) {
                if (vendor_ie->payload[CONNECT_ROUTER_STATUS] == 1) {
                    if (first_vendor_ie_tag == true || vendor_ie->payload[LEVEL] < ap_router->level) {
                        memcpy(ap_router->router_mac, (char*)sa, MAC_LEN);
                        ap_router->level = vendor_ie->payload[LEVEL];
                        ap_router->rssi  = (~((rssi & 0x7f) - 1)) & 0x7f;
                        ESP_LOGI(TAG, "router_level: %d rssi: -%d SoftAP MAC "MACSTR"", ap_router->level, ap_router->rssi, MAC2STR(ap_router->router_mac));
                        first_vendor_ie_tag = false;
                    }
                } else {
                    ESP_LOGD(TAG, "CONNECT_ROUTER_STATUS false\r\n");
                }
            } else {
                ESP_LOGD(TAG, "Station is MAX\r\n");
            }
        }
    }
    return;
}

void esp_gateway_vendor_ie_cb_deinit(void *ctx, wifi_vendor_ie_type_t type, const uint8_t sa[6], const vendor_ie_data_t *vnd_ie, int rssi)
{
    // TODO
    return;
}

void esp_gateway_vendor_ie_scan(void)
{
    uint16_t count = 0;
    uint16_t ap_channel = 0;
    ESP_ERROR_CHECK(esp_wifi_scan_start(NULL, true));
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
    if (ap_channel) {
        for (int i = 0; i < 5; ++i) {
            wifi_scan_config_t scanconf = {
                .channel = ap_channel,
            };
            ESP_ERROR_CHECK(esp_wifi_scan_start(&scanconf, true));
        }
    } else {
        ESP_ERROR_CHECK(esp_wifi_scan_start(NULL, true));
    }
}

void esp_gateway_vendor_ie_init(void)
{
    ap_router = malloc(sizeof(ap_router_t));
    memset(ap_router, 0, sizeof(*ap_router));

    esp_gateway_vendor_ie = malloc(sizeof(vendor_ie_data_t) + (VENDOR_IE_DATA_LENGTH * sizeof(uint8_t)));
    memset(esp_gateway_vendor_ie, 0, sizeof(*esp_gateway_vendor_ie));
    esp_gateway_vendor_ie->element_id = WIFI_VENDOR_IE_ELEMENT_ID;
    esp_gateway_vendor_ie->length = VENDOR_IE_DATA_LENGTH;
    esp_gateway_vendor_ie->vendor_oui[0] = VENDOR_OUI_0;
    esp_gateway_vendor_ie->vendor_oui[1] = VENDOR_OUI_1;
    esp_gateway_vendor_ie->vendor_oui[2] = VENDOR_OUI_2;

    ESP_ERROR_CHECK(esp_wifi_set_vendor_ie(true, WIFI_VND_IE_TYPE_BEACON, WIFI_VND_IE_ID_0, esp_gateway_vendor_ie));
    esp_gateway_vendor_status = true;
    ESP_ERROR_CHECK(esp_wifi_set_vendor_ie_cb((esp_vendor_ie_cb_t)esp_gateway_vendor_ie_cb, NULL));

    esp_gateway_vendor_ie_scan();

    /* Update vendor_ie info */
    esp_gateway_vendor_ie_set_max_connect_number(8, false);
    esp_gateway_vendor_ie_set_station_number(0, false);
    esp_gateway_vendor_ie_set_rssi(ap_router->rssi, false);
    esp_gateway_vendor_ie_set_level(ap_router->level + 1, false);
    esp_gateway_vendor_ie_set_connect_status(0, true);
}

void esp_gateway_vendor_ie_set_max_connect_number(uint8_t max_connect_number, bool enable)
{
    if (esp_gateway_vendor_status) {
        ESP_ERROR_CHECK(esp_wifi_set_vendor_ie(false, WIFI_VND_IE_TYPE_BEACON, WIFI_VND_IE_ID_0, esp_gateway_vendor_ie));
        esp_gateway_vendor_status = false;
    }
    esp_gateway_vendor_ie->payload[MAX_CONNECT_NUMBER] = max_connect_number;
    if (enable) {
        ESP_ERROR_CHECK(esp_wifi_set_vendor_ie(true, WIFI_VND_IE_TYPE_BEACON, WIFI_VND_IE_ID_0, esp_gateway_vendor_ie));
        esp_gateway_vendor_status = true;
    }
}

void esp_gateway_vendor_ie_set_station_number(uint8_t station_number, bool enable)
{
    if (esp_gateway_vendor_status) {
        ESP_ERROR_CHECK(esp_wifi_set_vendor_ie(false, WIFI_VND_IE_TYPE_BEACON, WIFI_VND_IE_ID_0, esp_gateway_vendor_ie));
        esp_gateway_vendor_status = false;
    }
    esp_gateway_vendor_ie->payload[STATION_NUMBER] = station_number;
    if (enable) {
        ESP_ERROR_CHECK(esp_wifi_set_vendor_ie(true, WIFI_VND_IE_TYPE_BEACON, WIFI_VND_IE_ID_0, esp_gateway_vendor_ie));
        esp_gateway_vendor_status = true;
    }
}

void esp_gateway_vendor_ie_set_rssi(uint8_t rssi, bool enable)
{
    if (esp_gateway_vendor_status) {
        ESP_ERROR_CHECK(esp_wifi_set_vendor_ie(false, WIFI_VND_IE_TYPE_BEACON, WIFI_VND_IE_ID_0, esp_gateway_vendor_ie));
        esp_gateway_vendor_status = false;
    }
    esp_gateway_vendor_ie->payload[ROUTER_RSSI] = rssi;
    if (enable) {
        ESP_ERROR_CHECK(esp_wifi_set_vendor_ie(true, WIFI_VND_IE_TYPE_BEACON, WIFI_VND_IE_ID_0, esp_gateway_vendor_ie));
        esp_gateway_vendor_status = true;
    }
}

void esp_gateway_vendor_ie_set_level(uint8_t level, bool enable)
{
    if (esp_gateway_vendor_status) {
        ESP_ERROR_CHECK(esp_wifi_set_vendor_ie(false, WIFI_VND_IE_TYPE_BEACON, WIFI_VND_IE_ID_0, esp_gateway_vendor_ie));
        esp_gateway_vendor_status = false;
    }
    esp_gateway_vendor_ie->payload[LEVEL] = level;
    if (enable) {
        ESP_ERROR_CHECK(esp_wifi_set_vendor_ie(true, WIFI_VND_IE_TYPE_BEACON, WIFI_VND_IE_ID_0, esp_gateway_vendor_ie));
        esp_gateway_vendor_status = true;
    }
}

void esp_gateway_vendor_ie_set_connect_status(uint8_t connect_status, bool enable)
{
    if (esp_gateway_vendor_status) {
        ESP_ERROR_CHECK(esp_wifi_set_vendor_ie(false, WIFI_VND_IE_TYPE_BEACON, WIFI_VND_IE_ID_0, esp_gateway_vendor_ie));
        esp_gateway_vendor_status = false;
    }
    esp_gateway_vendor_ie->payload[CONNECT_ROUTER_STATUS] = connect_status;
    if (enable) {
        ESP_ERROR_CHECK(esp_wifi_set_vendor_ie(true, WIFI_VND_IE_TYPE_BEACON, WIFI_VND_IE_ID_0, esp_gateway_vendor_ie));
        esp_gateway_vendor_status = true;
    }
}

uint8_t esp_gateway_vendor_ie_get_max_connect_number(void)
{
    return esp_gateway_vendor_ie->payload[MAX_CONNECT_NUMBER];
}

uint8_t esp_gateway_vendor_ie_get_station_number(void)
{
    return esp_gateway_vendor_ie->payload[STATION_NUMBER];
}

uint8_t esp_gateway_vendor_ie_get_rssi(void)
{
    return esp_gateway_vendor_ie->payload[ROUTER_RSSI];
}

uint8_t esp_gateway_vendor_ie_get_level(void)
{
    return esp_gateway_vendor_ie->payload[LEVEL];
}

uint8_t esp_gateway_vendor_ie_get_connect_status(void)
{
    return esp_gateway_vendor_ie->payload[CONNECT_ROUTER_STATUS];
}

#endif // SET_VENDOR_IE
