// Copyright 2020-2021 Espressif Systems (Shanghai) PTE LTD
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

#pragma once

#include <esp_err.h>
#include "esp_wifi.h"

#include "lwip/opt.h"
#include "lwip/err.h"
#include "lwip/sys.h"
#include "lwip/lwip_napt.h"

#ifdef __cplusplus
extern "C"
{
#endif

esp_err_t esp_netif_up(esp_netif_t *esp_netif);

esp_netif_t* esp_gateway_create_netif(esp_netif_config_t* config, esp_netif_ip_info_t* ip_info, uint8_t mac[6], bool enable_dhcps);

esp_err_t esp_geteway_netif_list_add(esp_netif_t* netif);
esp_err_t esp_geteway_netif_list_remove(esp_netif_t* netif);
esp_err_t esp_geteway_netif_request_ip(esp_netif_ip_info_t* ip_info);
esp_err_t esp_geteway_netif_request_mac(uint8_t* mac);
#ifdef __cplusplus
}
#endif