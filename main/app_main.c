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

#include <stdio.h>
#include <string.h>
#include <stdlib.h>

#include "freertos/FreeRTOS.h"
#include "freertos/queue.h"
#include "freertos/task.h"

#include "esp_system.h"
#include "esp_log.h"
#include "esp_spi_flash.h"
#include "nvs_flash.h"
#include "nvs.h"
#include "driver/gpio.h"

#include "esp_netif.h"

#include "esp_eth.h"

#include "esp_storage.h"
#include "esp_gateway_wifi.h"
#include "esp_gateway_eth.h"
#include "esp_gateway_modem.h"
#include "esp_gateway_netif_virtual.h"

#include "led_gpio.h"
#include "button.h"

#define GPIO_LED_WIFI  CONFIG_GPIO_LED_WIFI
#define GPIO_LED_MODEM CONFIG_GPIO_LED_MODEM
#define GPIO_LED_ETH   CONFIG_GPIO_LED_ETH

#define GPIO_BUTTON_SW1 CONFIG_GPIO_BUTTON_SW1

#define MODEM_IS_OPEN CONFIG_MODEM_IS_OPEN

#define ESP_GATEWAY_WIFI_ROUTER_STA_SSID     CONFIG_WIFI_ROUTER_STA_SSID
#define ESP_GATEWAY_WIFI_ROUTER_STA_PASSWORD CONFIG_WIFI_ROUTER_STA_PASSWORD
#define ESP_GATEWAY_WIFI_ROUTER_AP_SSID      CONFIG_WIFI_ROUTER_AP_SSID
#define ESP_GATEWAY_WIFI_ROUTER_AP_PASSWORD  CONFIG_WIFI_ROUTER_AP_PASSWORD
#define ESP_GATEWAY_4G_ROUTER_AP_SSID        CONFIG_4G_ROUTER_AP_SSID
#define ESP_GATEWAY_4G_ROUTER_AP_PASSWORD    CONFIG_4G_ROUTER_AP_PASSWORD
#define ESP_GATEWAY_ETH_STA_SSID             CONFIG_ETH_STA_SSID
#define ESP_GATEWAY_ETH_STA_PASSWORD         CONFIG_ETH_STA_PASSWORD

#define ESP_GATEWAY_AP_CUSTOM_IP             CONFIG_AP_CUSTOM_IP
#define ESP_GATEWAY_AP_STATIC_IP_ADDR        CONFIG_AP_STATIC_IP_ADDR
#define ESP_GATEWAY_AP_STATIC_GW_ADDR        CONFIG_AP_STATIC_GW_ADDR
#define ESP_GATEWAY_AP_STATIC_NETMASK_ADDR   CONFIG_AP_STATIC_NETMASK_ADDR

typedef enum {
    FEAT_TYPE_WIFI,
    FEAT_TYPE_MODEM,
    FEAT_TYPE_ETH,
    FEAT_TYPE_MAX,
} feat_type_t;

static feat_type_t g_feat_type = FEAT_TYPE_WIFI;
static led_handle_t g_led_handle_list[3] = {NULL};

static const char *TAG = "main";

void button_tap_cb(void *arg)
{
    led_gpio_state_write(g_led_handle_list[g_feat_type], LED_GPIO_OFF);
    g_feat_type = (g_feat_type + 1) % FEAT_TYPE_MAX;
    ESP_LOGI(TAG, "button tap, g_feat_type: %d", g_feat_type);
    led_gpio_state_write(g_led_handle_list[g_feat_type], LED_GPIO_ON);
}

static void button_press_3sec_cb(void *arg)
{
    ESP_LOGI(TAG, "button_press_3sec_cb");

    esp_storage_set("feat_type", &g_feat_type, sizeof(feat_type_t));
    led_gpio_state_write(g_led_handle_list[g_feat_type], LED_GPIO_QUICK_BLINK);

    vTaskDelay(pdMS_TO_TICKS(2000));
    esp_restart();
}

void app_main(void)
{
    esp_log_level_set("*", ESP_LOG_INFO);

    esp_storage_init();
    esp_storage_get("feat_type", &g_feat_type, sizeof(feat_type_t));

    ESP_ERROR_CHECK(esp_netif_init());
    ESP_ERROR_CHECK(esp_event_loop_create_default());

    ESP_LOGI(TAG, "feat_type: %d", g_feat_type);

    g_led_handle_list[FEAT_TYPE_WIFI] = led_gpio_create(GPIO_LED_WIFI, LED_GPIO_DARK_HIGH);
    g_led_handle_list[FEAT_TYPE_ETH]  = led_gpio_create(GPIO_LED_ETH, LED_GPIO_DARK_HIGH);
    g_led_handle_list[FEAT_TYPE_MODEM] = led_gpio_create(GPIO_LED_MODEM, LED_GPIO_DARK_HIGH);

    led_gpio_state_write(g_led_handle_list[g_feat_type], LED_GPIO_ON);

    button_handle_t button_handle = button_create((gpio_num_t)GPIO_BUTTON_SW1, BUTTON_ACTIVE_LOW);

    button_add_tap_cb(button_handle, BUTTON_CB_TAP, button_tap_cb, NULL);
    button_add_press_cb(button_handle, 3, button_press_3sec_cb, NULL);

    switch (g_feat_type) {
        case FEAT_TYPE_WIFI:
            ESP_LOGI(TAG, "============================");
            ESP_LOGI(TAG, "Wi-Fi Repeater");
            ESP_LOGI(TAG, "============================");

            /* Create STA netif */
            esp_netif_t *sta_wifi_netif = esp_gateway_wifi_init(WIFI_MODE_STA);
            esp_gateway_wifi_set(WIFI_MODE_STA, ESP_GATEWAY_WIFI_ROUTER_STA_SSID, ESP_GATEWAY_WIFI_ROUTER_STA_PASSWORD);
            esp_gateway_wifi_sta_connected(portMAX_DELAY);

            /* Create AP netif  */
            esp_netif_t *ap_wifi_netif = esp_netif_create_default_wifi_ap();

#if ESP_GATEWAY_AP_CUSTOM_IP
            esp_gateway_set_custom_ip_network_segment(ap_wifi_netif, ESP_GATEWAY_AP_STATIC_IP_ADDR, ESP_GATEWAY_AP_STATIC_GW_ADDR, ESP_GATEWAY_AP_STATIC_NETMASK_ADDR);
#endif

            /* Config dns info for AP */
            esp_netif_dns_info_t dns;
            ESP_ERROR_CHECK(esp_netif_get_dns_info(sta_wifi_netif, ESP_NETIF_DNS_MAIN, &dns));
            ESP_LOGI(TAG, "Main DNS: " IPSTR, IP2STR(&dns.ip.u_addr.ip4));
            ESP_ERROR_CHECK(esp_gateway_wifi_set_dhcps(ap_wifi_netif, dns.ip.u_addr.ip4.addr));

            ESP_ERROR_CHECK(esp_wifi_set_mode(WIFI_MODE_APSTA));
            esp_gateway_wifi_set(WIFI_MODE_AP, ESP_GATEWAY_WIFI_ROUTER_AP_SSID, ESP_GATEWAY_WIFI_ROUTER_AP_PASSWORD);
            vTaskDelay(pdMS_TO_TICKS(100));

            esp_gateway_wifi_napt_enable();
            break;

        case FEAT_TYPE_MODEM: {
#if MODEM_IS_OPEN
            ESP_LOGI(TAG, "============================");
            ESP_LOGI(TAG, "4G Router");
            ESP_LOGI(TAG, "============================");

            esp_netif_t *ppp_netif = esp_gateway_modem_init();
            esp_netif_t *ap_netif  = esp_gateway_wifi_init(WIFI_MODE_AP);

#if ESP_GATEWAY_AP_CUSTOM_IP
            esp_gateway_set_custom_ip_network_segment(ap_wifi_netif, ESP_GATEWAY_AP_STATIC_IP_ADDR, ESP_GATEWAY_AP_STATIC_GW_ADDR, ESP_GATEWAY_AP_STATIC_NETMASK_ADDR);
#endif

            esp_netif_dns_info_t dns;
            ESP_ERROR_CHECK(esp_netif_get_dns_info(ppp_netif, ESP_NETIF_DNS_MAIN, &dns));
            ESP_ERROR_CHECK(esp_gateway_wifi_set_dhcps(ap_netif, dns.ip.u_addr.ip4.addr));

            esp_gateway_wifi_set(WIFI_MODE_AP, ESP_GATEWAY_4G_ROUTER_AP_SSID, ESP_GATEWAY_4G_ROUTER_AP_PASSWORD);
            vTaskDelay(pdMS_TO_TICKS(100));

            esp_gateway_wifi_napt_enable();
#endif
            break;
        }

        case FEAT_TYPE_ETH:
#if CONFIG_IDF_TARGET_ESP32
            ESP_LOGI(TAG, "============================");
            ESP_LOGI(TAG, "ETH Router");
            ESP_LOGI(TAG, "============================");

            if (gpio_get_level((gpio_num_t)GPIO_BUTTON_SW1)) {
                esp_gateway_wifi_ap_init();
            } else {
                esp_gateway_wifi_init(WIFI_MODE_STA);
                esp_gateway_wifi_set(WIFI_MODE_STA, ESP_GATEWAY_ETH_STA_SSID, ESP_GATEWAY_ETH_STA_PASSWORD);
            }

            esp_gateway_eth_init();

            esp_gateway_netif_virtual_init();
#endif
            break;

        default:
            break;
    }
}
