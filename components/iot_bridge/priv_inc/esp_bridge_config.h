// Copyright 2022 Espressif Systems (Shanghai) PTE LTD
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

/**
 * @brief Maximum MAC size
 *
 */
#define ESP_BRIDGE_MAC_MAX_LEN     (6)

/**
 * @brief Maximum SSID size
 *
 */
#define ESP_BRIDGE_SSID_MAX_LEN    (32)

#ifdef CONFIG_ESP_BRIDGE_SOFTAP_MAX_CONNECT_NUMBER
#define ESP_BRIDGE_SOFTAP_MAX_CONNECT_NUMBER CONFIG_ESP_BRIDGE_SOFTAP_MAX_CONNECT_NUMBER
#else
#define ESP_BRIDGE_SOFTAP_MAX_CONNECT_NUMBER 8
#endif

enum {
    ESP_BRIDGE_EXTERNAL_NETIF_INVALID = -1,
#ifdef CONFIG_BRIDGE_EXTERNAL_NETIF_STATION
    ESP_BRIDGE_EXTERNAL_NETIF_STATION,
#endif
#ifdef CONFIG_BRIDGE_EXTERNAL_NETIF_MODEM
    ESP_BRIDGE_EXTERNAL_NETIF_MODEM,
#endif
#ifdef CONFIG_BRIDGE_EXTERNAL_NETIF_ETHERNET
    ESP_BRIDGE_EXTERNAL_NETIF_ETHERNET,
#endif
    ESP_BRIDGE_EXTERNAL_NETIF_MAX
};

#ifdef __cplusplus
}
#endif
