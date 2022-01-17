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
#include "esp_wifi.h"
#include "esp_netif.h"
#include "esp_eth.h"

#include "esp_storage.h"
#include "esp_gateway_wifi.h"
#include "esp_gateway_eth.h"
#include "esp_gateway_modem.h"
#include "esp_gateway_vendor_ie.h"
#include "esp_gateway_netif_virtual.h"

#include "web_server.h"
#include "led_strip.h"
#include "led_gpio.h"
#include "button.h"
#include "esp_gateway_config.h"

#if SET_VENDOR_IE
vendor_ie_data_t *esp_gateway_vendor_ie;
ap_router_t *ap_router;
#endif // SET_VENDOR_IE
char router_mac[MAC_LEN] = {0};

#if CONFIG_IDF_TARGET_ESP32C3
static led_strip_t *pStrip_a;
static uint8_t s_led_state = 0;
static uint32_t blink_period = 1;
#endif // CONFIG_IDF_TARGET_ESP32C3
feat_type_t g_feat_type = FEAT_TYPE_WIFI;
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

#if CONFIG_IDF_TARGET_ESP32C3
static void configure_led_strip(void)
{
    ESP_LOGI(TAG, "Example configured to blink addressable LED!");
    /* LED strip initialization with the GPIO and pixels number*/
    pStrip_a = led_strip_init(CONFIG_BLINK_LED_RMT_CHANNEL, GPIO_BLINK, 1);
    /* Set all LED off to clear all pixels */
    pStrip_a->clear(pStrip_a, 50);
}

static void blink_led(void)
{
    /* If the addressable LED is enabled */
    if (s_led_state) {
        /* Set the LED pixel using RGB from 0 (0%) to 255 (100%) for each color */
        pStrip_a->set_pixel(pStrip_a, 0, 16, 16, 16);
        /* Refresh the strip to send data */
        pStrip_a->refresh(pStrip_a, 100);
    } else {
        /* Set all LED off to clear all pixels */
        pStrip_a->clear(pStrip_a, 50);
    }
}
#endif // CONFIG_IDF_TARGET_ESP32C3

void app_main(void)
{
    esp_log_level_set("*", ESP_LOG_INFO);

    esp_storage_init();
    esp_storage_get("feat_type", &g_feat_type, sizeof(feat_type_t));

    ESP_ERROR_CHECK(esp_netif_init());
    ESP_ERROR_CHECK(esp_event_loop_create_default());

    ESP_LOGI(TAG, "feat_type: %d", g_feat_type);
#if CONFIG_IDF_TARGET_ESP32C3
    /* Configure the peripheral according to the LED type */
    configure_led_strip();
#endif // CONFIG_IDF_TARGET_ESP32C3
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

#if SET_VENDOR_IE
            ap_router = malloc(sizeof(ap_router_t));
            memset(ap_router, 0, sizeof(*ap_router));
            esp_gateway_vendor_ie = esp_wifi_vendor_ie_init();
            ESP_ERROR_CHECK(esp_wifi_set_vendor_ie(true, WIFI_VND_IE_TYPE_BEACON, WIFI_VND_IE_ID_0, esp_gateway_vendor_ie));
            ESP_ERROR_CHECK(esp_wifi_set_vendor_ie_cb((esp_vendor_ie_cb_t)esp_gateway_vendor_ie_cb, NULL));

            for (int i = 0; i < 2; i++) {
                ESP_ERROR_CHECK(esp_wifi_scan_start(NULL, true));
            }
            ESP_ERROR_CHECK(esp_wifi_scan_stop());

            if (ap_router->level != WIFI_ROUTER_LEVEL_0) {
                ESP_LOGI(TAG, "wifi_router_level: %d", ap_router->level);
                esp_gateway_wifi_set(WIFI_MODE_STA, ESP_GATEWAY_WIFI_ROUTER_AP_SSID, ESP_GATEWAY_WIFI_ROUTER_AP_PASSWORD, ap_router->router_mac);
            } else {
                ESP_LOGI(TAG, "wifi_router_level: %d", ap_router->level);
                esp_gateway_wifi_set(WIFI_MODE_STA, ESP_GATEWAY_WIFI_ROUTER_STA_SSID, ESP_GATEWAY_WIFI_ROUTER_STA_PASSWORD, NULL);
            }
            esp_gateway_wifi_sta_connected(portMAX_DELAY);

            /* Update vendor_ie info */
            ESP_ERROR_CHECK(esp_wifi_set_vendor_ie(false, WIFI_VND_IE_TYPE_BEACON, WIFI_VND_IE_ID_0, esp_gateway_vendor_ie));
            (*esp_gateway_vendor_ie).payload[MAX_CONNECT_NUMBER] = 8;
            (*esp_gateway_vendor_ie).payload[STATION_NUMBER] = 0;
            (*esp_gateway_vendor_ie).payload[ROUTER_RSSI] = ap_router->rssi;
            (*esp_gateway_vendor_ie).payload[CONNECT_ROUTER_STATUS] = 1;
            (*esp_gateway_vendor_ie).payload[LEVEL] = ap_router->level + 1;
            ESP_ERROR_CHECK(esp_wifi_set_vendor_ie(true, WIFI_VND_IE_TYPE_BEACON, WIFI_VND_IE_ID_0, esp_gateway_vendor_ie));
#else
            esp_gateway_wifi_set(WIFI_MODE_STA, ESP_GATEWAY_WIFI_ROUTER_STA_SSID, ESP_GATEWAY_WIFI_ROUTER_STA_PASSWORD, NULL);
            esp_gateway_wifi_sta_connected(portMAX_DELAY);
#endif // SET_VENDOR_IE

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
            esp_gateway_wifi_set(WIFI_MODE_AP, ESP_GATEWAY_WIFI_ROUTER_AP_SSID, ESP_GATEWAY_WIFI_ROUTER_AP_PASSWORD, NULL);

            /* Get Channel */
            uint8_t ap_channel = 0, second_channel = 0;
            ESP_ERROR_CHECK(esp_wifi_get_channel(&ap_channel, (wifi_second_chan_t*)&second_channel));
            ESP_LOGI(TAG, "SoftAP channel: %d, Second channel: %d", ap_channel, second_channel);

#if SET_VENDOR_IE && CONFIG_IDF_TARGET_ESP32C3
            blink_period = (*esp_gateway_vendor_ie).payload[LEVEL];
#endif // CONFIG_IDF_TARGET_ESP32C3

            /* Enable napt */
            esp_gateway_wifi_napt_enable();
            esp_wifi_get_mac(ESP_IF_WIFI_AP, (uint8_t*)router_mac);
            ESP_LOGI(TAG, "SoftAP MAC "MACSTR"", MAC2STR(router_mac));
            break;

        case FEAT_TYPE_MODEM: {
#if MODEM_IS_OPEN
            ESP_LOGI(TAG, "============================");
            ESP_LOGI(TAG, "4G Router");
            ESP_LOGI(TAG, "============================");

            esp_netif_t *ppp_netif = esp_gateway_modem_init();
            esp_netif_t *ap_netif  = esp_gateway_wifi_init(WIFI_MODE_AP);

#if ESP_GATEWAY_AP_CUSTOM_IP
            esp_gateway_set_custom_ip_network_segment(ap_netif, ESP_GATEWAY_AP_STATIC_IP_ADDR, ESP_GATEWAY_AP_STATIC_GW_ADDR, ESP_GATEWAY_AP_STATIC_NETMASK_ADDR);
#endif

            esp_netif_dns_info_t dns;
            ESP_ERROR_CHECK(esp_netif_get_dns_info(ppp_netif, ESP_NETIF_DNS_MAIN, &dns));
            ESP_ERROR_CHECK(esp_gateway_wifi_set_dhcps(ap_netif, dns.ip.u_addr.ip4.addr));

            esp_gateway_wifi_set(WIFI_MODE_AP, ESP_GATEWAY_4G_ROUTER_AP_SSID, ESP_GATEWAY_4G_ROUTER_AP_PASSWORD, NULL);
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
                esp_gateway_wifi_set(WIFI_MODE_STA, ESP_GATEWAY_ETH_STA_SSID, ESP_GATEWAY_ETH_STA_PASSWORD, NULL);
            }

            esp_gateway_eth_init();

            esp_gateway_netif_virtual_init();
#endif
            break;

        default:
            break;
    }
	
	StartWebServer();

#if CONFIG_IDF_TARGET_ESP32C3
    while (1) {
        for (int num = 0; num < blink_period; num ++) {
            /* Toggle the LED state */
            s_led_state = !s_led_state;
            blink_led();
            vTaskDelay(150 / portTICK_PERIOD_MS);
            /* Toggle the LED state */
            s_led_state = !s_led_state;
            blink_led();
            vTaskDelay(200 / portTICK_PERIOD_MS);
        }
        vTaskDelay(1000 / portTICK_PERIOD_MS);
    }
#endif // CONFIG_IDF_TARGET_ESP32C3
}
