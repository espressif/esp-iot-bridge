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

#if CONFIG_SET_VENDOR_IE
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
} esp_gateway_wifi_router_level_t;

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
    char router_mac[ESP_GATEWAY_MAC_MAX_LEN];
} ap_router_t;

void esp_gateway_vendor_ie_scan(void);

void esp_gateway_vendor_ie_init(void);

/**
  * @brief     Function signature for received Vendor-Specific Information Element callback.
  * @param     ctx Context argument, as passed to esp_wifi_set_vendor_ie_cb() when registering callback.
  * @param     type Information element type, based on frame type received.
  * @param     sa Source 802.11 address.
  * @param     vnd_ie Pointer to the vendor specific element data received.
  * @param     rssi Received signal strength indication.
  */
void esp_gateway_vendor_ie_cb(void *ctx, wifi_vendor_ie_type_t type, const uint8_t sa[6], const vendor_ie_data_t *vnd_ie, int rssi);

/**
* @brief Set Max connect number for Vendor IE
*
* @param[in] max_connect_number: Max connect number
* @param[in] enable: Set true to enable specified IE, else, set it false
*/
void esp_gateway_vendor_ie_set_max_connect_number(uint8_t max_connect_number, bool enable);

/**
* @brief Set station number for Vendor IE
*
* @param[in] station_number: station number
* @param[in] enable: Set true to enable specified IE, else, set it false
*/
void esp_gateway_vendor_ie_set_station_number(uint8_t station_number, bool enable);

/**
* @brief Set rssi for Vendor IE
*
* @param[in] rssi: rssi
* @param[in] enable: Set true to enable specified IE, else, set it false
*/
void esp_gateway_vendor_ie_set_rssi(uint8_t rssi, bool enable);

/**
* @brief Set level for Vendor IE
*
* @param[in] level: node level
* @param[in] enable: Set true to enable specified IE, else, set it false
*/
void esp_gateway_vendor_ie_set_level(uint8_t level, bool enable);

/**
* @brief Set connect status for Vendor IE
*
* @param[in] connect_status: node connect status
* @param[in] enable: Set true to enable specified IE, else, set it false
*/
void esp_gateway_vendor_ie_set_connect_status(uint8_t connect_status, bool enable);

/**
  * @brief     Get Max connect number
  *
  * @return
  *   - uint8_t max_connect_number: Max connect number
  */
uint8_t esp_gateway_vendor_ie_get_max_connect_number(void);

/**
  * @brief     Get station number
  *
  * @return
  *   - uint8_t station_number: station number
  */
uint8_t esp_gateway_vendor_ie_get_station_number(void);

/**
  * @brief     Get rssi
  *
  * @return
  *   - uint8_t rssi: rssi
  */
uint8_t esp_gateway_vendor_ie_get_rssi(void);

/**
  * @brief     Get node level
  *
  * @return
  *   - uint8_t level: node level
  */
uint8_t esp_gateway_vendor_ie_get_level(void);

/**
  * @brief     Get connect status
  *
  * @return
  *   - uint8_t connect_status: connect status
  */
uint8_t esp_gateway_vendor_ie_get_connect_status(void);
#endif // CONFIG_SET_VENDOR_IE

#ifdef __cplusplus
}
#endif
