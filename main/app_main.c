// Copyright 2021-2022 Espressif Systems (Shanghai) PTE LTD
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
#include "wifi_prov_mgr.h"

#define GPIO_BUTTON_SW1    CONFIG_GPIO_BUTTON_SW1

static const char *TAG = "main";

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

#if defined(CONFIG_GATEWAY_USE_WIFI_PROVISIONING_OVER_BLE)
static bool wifi_prov_restart = false;

static void esp_gateway_wifi_prov_mgr_task(void *pvParameters)
{
    if (!wifi_provision_in_progress()) {
        esp_gateway_wifi_prov_mgr();
    } else {
        ESP_LOGI(TAG, "Wi-Fi Provisioning is still progress");
    }
    vTaskDelete(NULL);
}

static void button_press_3sec_cb(void *arg)
{
    ESP_LOGI(TAG, "Start Wi-Fi Provisioning");
    wifi_prov_restart = true;
}

static void button_release_3sec_cb(void *arg)
{
    if (wifi_prov_restart) {
        xTaskCreate(esp_gateway_wifi_prov_mgr_task, "esp_gateway_wifi_prov_mgr", 4096, NULL, 5, NULL);
        wifi_prov_restart = false;
    }
}
#endif /* CONFIG_GATEWAY_USE_WIFI_PROVISIONING_OVER_BLE */

static void button_press_6sec_cb(void *arg)
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
    button_add_press_cb(button_handle, 6, button_press_6sec_cb, NULL);

    esp_gateway_create_all_netif();

#if defined(CONFIG_GATEWAY_USE_WEB_SERVER)
    StartWebServer();
#endif /* CONFIG_GATEWAY_USE_WEB_SERVER */
#if defined(CONFIG_GATEWAY_USE_WIFI_PROVISIONING_OVER_BLE)
    button_add_press_cb(button_handle, 3, button_press_3sec_cb, NULL);
    button_add_release_cb(button_handle, 3, button_release_3sec_cb, NULL);
    esp_gateway_wifi_prov_mgr();
#endif /* CONFIG_GATEWAY_USE_WIFI_PROVISIONING_OVER_BLE */
}
