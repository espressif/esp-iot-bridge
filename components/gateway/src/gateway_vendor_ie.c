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

#include "esp_gateway_vendor_ie.h"

#if SET_VENDOR_IE

static const char *TAG = "vendor_ie";

bool first_vendor_ie_tag = true;

extern ap_router_t *ap_router;

void print_vendor_ie_info(vendor_ie_data_t *vendor_ie)
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
            // printf("MAC "MACSTR"", MAC2STR(ap_router->router_mac));
            // print_vendor_ie_info(vendor_ie);
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
                    printf("CONNECT_ROUTER_STATUS false\r\n");
                }
            } else {
                printf("Station is MAX\r\n");
            }
        }
    }
    return;
}

vendor_ie_data_t *esp_wifi_vendor_ie_init(void)
{
    vendor_ie_data_t *vendor_ie = malloc(sizeof(vendor_ie_data_t) + (VENDOR_IE_DATA_LENGTH * sizeof(uint8_t)));
    memset(vendor_ie, 0, sizeof(*vendor_ie));
    (*vendor_ie).element_id = WIFI_VENDOR_IE_ELEMENT_ID;
    (*vendor_ie).length = VENDOR_IE_DATA_LENGTH;
    (*vendor_ie).vendor_oui[0] = VENDOR_OUI_0;
    (*vendor_ie).vendor_oui[1] = VENDOR_OUI_1;
    (*vendor_ie).vendor_oui[2] = VENDOR_OUI_2;

    return vendor_ie;
}

void esp_gateway_vendor_ie_cb_deinit(void *ctx, wifi_vendor_ie_type_t type, const uint8_t sa[6], const vendor_ie_data_t *vnd_ie, int rssi)
{
    return;
}

#endif // SET_VENDOR_IE
