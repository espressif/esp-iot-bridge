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

#if defined(CONFIG_GATEWAY_EXTERNAL_NETIF_STATION)
esp_netif_t *esp_gateway_create_station_netif(esp_netif_ip_info_t* ip_info, uint8_t mac[6], bool data_forwarding, bool enable_dhcps);
#endif

#if defined(CONFIG_GATEWAY_DATA_FORWARDING_NETIF_SOFTAP)
esp_netif_t *esp_gateway_create_softap_netif(esp_netif_ip_info_t* ip_info, uint8_t mac[6], bool data_forwarding, bool enable_dhcps);
#endif

#if defined(CONFIG_GATEWAY_EXTERNAL_NETIF_ETHERNET) || defined(CONFIG_GATEWAY_DATA_FORWARDING_NETIF_ETHERNET)
esp_netif_t* esp_gateway_create_eth_netif(esp_netif_ip_info_t* ip_info, uint8_t mac[6], bool data_forwarding, bool enable_dhcps);
#endif

#if defined(CONFIG_GATEWAY_DATA_FORWARDING_NETIF_SDIO)
esp_netif_t* esp_gateway_create_sdio_netif(esp_netif_ip_info_t* ip_info, uint8_t mac[6], bool data_forwarding, bool enable_dhcps);
#endif

#if defined(CONFIG_GATEWAY_DATA_FORWARDING_NETIF_SPI)
esp_netif_t* esp_gateway_create_spi_netif(esp_netif_ip_info_t* ip_info, uint8_t mac[6], bool data_forwarding, bool enable_dhcps);
#endif

#if defined(CONFIG_GATEWAY_EXTERNAL_NETIF_MODEM)
esp_netif_t* esp_gateway_create_modem_netif(esp_netif_ip_info_t* ip_info, uint8_t mac[6], bool data_forwarding, bool enable_dhcps);
#endif

void esp_gateway_create_all_netif(void);