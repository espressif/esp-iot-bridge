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

#include "nvs.h"
#include "nvs_flash.h"
#include "esp_log.h"
#include "esp_netif.h"
#include "esp_system.h"
#include "esp_event.h"

#include "esp_gateway.h"

#include "button.h"
#include "web_server.h"
#include "led_strip.h"

#define GPIO_BLINK         CONFIG_GPIO_BLINK
#define GPIO_BUTTON_SW1    CONFIG_GPIO_BUTTON_SW1


static const char *TAG = "main";

static led_strip_t *pStrip_a;
static uint8_t s_led_state = 0;
static uint32_t blink_period = 1;
static void configure_led_strip(void)
{
    ESP_LOGI(TAG, "Configured to blink addressable LED!");
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

static esp_err_t esp_storage_init(void)
{
    esp_err_t ret = nvs_flash_init();

    if (ret == ESP_ERR_NVS_NO_FREE_PAGES || ret == ESP_ERR_NVS_NEW_VERSION_FOUND) {
        // NVS partition was truncated and needs to be erased
        // Retry nvs_flash_init
        ESP_ERROR_CHECK(nvs_flash_erase());
        ret = nvs_flash_init();
    }

    return ret;
}

static void button_press_3sec_cb(void *arg)
{
    ESP_LOGI(TAG, "Restore factory settings");
    nvs_flash_erase();
    esp_restart();
}

void app_main(void)
{
    esp_log_level_set("*", ESP_LOG_INFO);

    esp_storage_init();

    ESP_ERROR_CHECK(esp_netif_init());
    ESP_ERROR_CHECK(esp_event_loop_create_default());

    button_handle_t button_handle = button_create((gpio_num_t)GPIO_BUTTON_SW1, BUTTON_ACTIVE_LOW);
    button_add_press_cb(button_handle, 3, button_press_3sec_cb, NULL);

    /* Configure the peripheral according to the LED type */
    configure_led_strip();

    esp_gateway_create_all_netif();

#if defined(CONFIG_GATEWAY_EXTERNAL_NETIF_STATION)
	StartWebServer();
#endif

#if SET_VENDOR_IE
    blink_period = esp_gateway_vendor_ie_get_level();
#endif
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
}
