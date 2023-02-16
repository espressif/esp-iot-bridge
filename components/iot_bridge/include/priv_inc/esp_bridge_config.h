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
#define BRIDGE_MAC_MAX_LEN     (6)

/**
 * @brief Maximum SSID size
 *
 */
#define BRIDGE_SSID_MAX_LEN    (32)

#ifdef CONFIG_BRIDGE_SOFTAP_MAX_CONNECT_NUMBER
#define BRIDGE_SOFTAP_MAX_CONNECT_NUMBER CONFIG_BRIDGE_SOFTAP_MAX_CONNECT_NUMBER
#else
#define BRIDGE_SOFTAP_MAX_CONNECT_NUMBER 8
#endif

enum {
    BRIDGE_EXTERNAL_NETIF_INVALID = -1,
#ifdef CONFIG_BRIDGE_EXTERNAL_NETIF_STATION
    BRIDGE_EXTERNAL_NETIF_STATION,
#endif
#ifdef CONFIG_BRIDGE_EXTERNAL_NETIF_MODEM
    BRIDGE_EXTERNAL_NETIF_MODEM,
#endif
#ifdef CONFIG_BRIDGE_EXTERNAL_NETIF_ETHERNET
    BRIDGE_EXTERNAL_NETIF_ETHERNET,
#endif
    BRIDGE_EXTERNAL_NETIF_MAX
};

#ifdef __cplusplus
}
#endif
