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

#define GPIO_BLINK      CONFIG_GPIO_BLINK
#define GPIO_BUTTON_SW1 CONFIG_GPIO_BUTTON_SW1
#define GPIO_LED_WIFI   CONFIG_GPIO_LED_WIFI
#define GPIO_LED_MODEM  CONFIG_GPIO_LED_MODEM
#define GPIO_LED_ETH    CONFIG_GPIO_LED_ETH

#define MODEM_IS_OPEN CONFIG_MODEM_IS_OPEN
#define SET_VENDOR_IE CONFIG_SET_VENDOR_IE

#define ESP_GATEWAY_WIFI_ROUTER_STA_SSID     CONFIG_WIFI_ROUTER_STA_SSID
#define ESP_GATEWAY_WIFI_ROUTER_STA_PASSWORD CONFIG_WIFI_ROUTER_STA_PASSWORD
#define ESP_GATEWAY_WIFI_ROUTER_AP_SSID      CONFIG_WIFI_ROUTER_AP_SSID
#define ESP_GATEWAY_WIFI_ROUTER_AP_PASSWORD  CONFIG_WIFI_ROUTER_AP_PASSWORD
#define ESP_GATEWAY_4G_ROUTER_AP_SSID        CONFIG_4G_ROUTER_AP_SSID
#define ESP_GATEWAY_4G_ROUTER_AP_PASSWORD    CONFIG_4G_ROUTER_AP_PASSWORD
#define ESP_GATEWAY_ETH_STA_SSID             CONFIG_ETH_STA_SSID
#define ESP_GATEWAY_ETH_STA_PASSWORD         CONFIG_ETH_STA_PASSWORD
#define ESP_GATEWAY_ETH_AP_SSID              CONFIG_ETH_ROUTER_WIFI_SSID
#define ESP_GATEWAY_ETH_AP_PASSWORD          CONFIG_ETH_ROUTER_WIFI_PASSWORD
#define ESP_GATEWAY_ETH_ROUTER_WIFI_CHANNEL  CONFIG_ETH_ROUTER_WIFI_CHANNEL
#define ESP_GATEWAY_ETH_ROUTER_MAX_STA_CONN  CONFIG_ETH_ROUTER_MAX_STA_CONN

#ifdef CONFIG_AP_CUSTOM_IP
#define ESP_GATEWAY_AP_CUSTOM_IP             CONFIG_AP_CUSTOM_IP
#define ESP_GATEWAY_AP_STATIC_IP_ADDR        CONFIG_AP_STATIC_IP_ADDR
#define ESP_GATEWAY_AP_STATIC_GW_ADDR        CONFIG_AP_STATIC_GW_ADDR
#define ESP_GATEWAY_AP_STATIC_NETMASK_ADDR   CONFIG_AP_STATIC_NETMASK_ADDR
#endif

typedef enum {
    FEAT_TYPE_WIFI,
    FEAT_TYPE_MODEM,
    FEAT_TYPE_ETH,
    FEAT_TYPE_MAX,
} feat_type_t;