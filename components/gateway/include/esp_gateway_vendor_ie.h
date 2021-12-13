// Copyright 2021 Espressif Systems (Shanghai) PTE LTD
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

#pragma once

#ifdef __cplusplus
extern "C"
{
#endif

#include "esp_wifi_types.h"
#include "esp_gateway_config.h"

#define MAC_LEN 6

#if SET_VENDOR_IE
#define VENDOR_OUI_0 CONFIG_VENDOR_OUI_0
#define VENDOR_OUI_1 CONFIG_VENDOR_OUI_1
#define VENDOR_OUI_2 CONFIG_VENDOR_OUI_2

#define VENDOR_IE_DATA_LENGTH_MINIMUM 4
#define VENDOR_IE_DATA_LENGTH (VENDOR_IE_DATA_LENGTH_MINIMUM + VENDOR_IE_MAX)

typedef enum {
    WIFI_ROUTER_LEVEL_0 = 0,
    WIFI_ROUTER_LEVEL_1,
    WIFI_ROUTER_LEVEL_2,
    WIFI_ROUTER_LEVEL_3,
    WIFI_ROUTER_LEVEL_4,
    WIFI_ROUTER_LEVEL_5,
    WIFI_ROUTER_LEVEL_6,
} esp_geteway_wifi_router_level_t;

typedef enum {
    MAX_CONNECT_NUMBER = 0,
    STATION_NUMBER,
    ROUTER_RSSI,
    CONNECT_ROUTER_STATUS,
    LEVEL,
    VENDOR_IE_MAX,
} esp_gateway_vendor_ie_t;

typedef struct {
    uint8_t level;
    uint8_t rssi;
    char router_mac[MAC_LEN];
} ap_router_t;

void esp_gateway_vendor_ie_cb(void *ctx, wifi_vendor_ie_type_t type, const uint8_t sa[6], const vendor_ie_data_t *vnd_ie, int rssi);

vendor_ie_data_t *esp_wifi_vendor_ie_init(void);
#endif // SET_VENDOR_IE

#ifdef __cplusplus
}
#endif
